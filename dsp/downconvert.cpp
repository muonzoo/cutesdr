// downconvert.cpp: implementation of the CDownConvert class.
//
//  This class takes I/Q baseband data and performs tuning
//(Frequency shifting of the baseband signal) as well as
// decimation in powers of 2 after the shifting.
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
//	2011-04-20  Changed some scope resolution operators to allow compiling with different compilers
//////////////////////////////////////////////////////////////////////
//==========================================================================================
// + + +   This Software is released under the "Simplified BSD License"  + + +
//Copyright 2010 Moe Wheatley. All rights reserved.
//
//Redistribution and use in source and binary forms, with or without modification, are
//permitted provided that the following conditions are met:
//
//   1. Redistributions of source code must retain the above copyright notice, this list of
//	  conditions and the following disclaimer.
//
//   2. Redistributions in binary form must reproduce the above copyright notice, this list
//	  of conditions and the following disclaimer in the documentation and/or other materials
//	  provided with the distribution.
//
//THIS SOFTWARE IS PROVIDED BY Moe Wheatley ``AS IS'' AND ANY EXPRESS OR IMPLIED
//WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
//FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL Moe Wheatley OR
//CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
//ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
//ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//The views and conclusions contained in the software and documentation are those of the
//authors and should not be interpreted as representing official policies, either expressed
//or implied, of Moe Wheatley.
//==========================================================================================
#include "dsp/downconvert.h"
#include "dsp/filtercoef.h"
#include "gui/testbench.h"
#include "interface/perform.h"
#include <QDebug>

//pick a method of calculating the NCO
#define NCO_LIB 0		//normal sin cos library (188nS)
#define NCO_OSC 1		//quadrature oscillator (25nS)
#define NCO_VCASM 0		//Visual C assembly call to floating point sin/cos instruction
#define NCO_GCCASM 0	//GCC assembly call to floating point sin/cos instruction (100nS)

#define MIN_OUTPUT_RATE (7900.0*2.0)

#define MAX_HALF_BAND_BUFSIZE 32768


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CDownConvert::CDownConvert()
{
int i;
	m_NcoInc = 0.0;
	m_NcoTime = 0.0;
	m_NcoFreq = 0.0;
	m_CW_Offset = 0.0;
	m_InRate = 100000.0;
	m_MaxBW = 10000.0;
	for(i=0; i<MAX_DECSTAGES; i++)
		m_pDecimatorPtrs[i] = NULL;
	m_Osc1.re = 1.0;	//initialize unit vector that will get rotated
	m_Osc1.im = 0.0;
}

CDownConvert::~CDownConvert()
{
	DeleteFilters();
}

//////////////////////////////////////////////////////////////////////
// Delete all active Filters in m_pDecimatorPtrs array
//////////////////////////////////////////////////////////////////////
void CDownConvert::DeleteFilters()
{
	for(int i=0; i<MAX_DECSTAGES; i++)
	{
		if(m_pDecimatorPtrs[i])
		{
			delete m_pDecimatorPtrs[i];
			m_pDecimatorPtrs[i] = NULL;
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Sets NCO Frequency parameters
//////////////////////////////////////////////////////////////////////
void CDownConvert::SetFrequency(TYPEREAL NcoFreq)
{
TYPEREAL tmpf = NcoFreq + m_CW_Offset;

	m_NcoFreq = tmpf;
	m_NcoInc = K_2PI*m_NcoFreq/m_InRate;
	m_OscCos = cos(m_NcoInc);
	m_OscSin = sin(m_NcoInc);
//qDebug()<<"NCO "<<m_NcoFreq;
}

//////////////////////////////////////////////////////////////////////
// Calculates sequence and number of decimation stages based on
// input sample rate and desired output bandwidth.  Returns final output rate
//from divide by 2 stages.
//////////////////////////////////////////////////////////////////////
TYPEREAL CDownConvert::SetDataRate(TYPEREAL InRate, TYPEREAL MaxBW)
{
int n = 0;
TYPEREAL f = InRate;
	if( (m_InRate!=InRate) ||
		(m_MaxBW!=MaxBW) )
	{
		m_InRate = InRate;
		m_MaxBW = MaxBW;
qDebug()<<"Inrate="<<m_InRate<<" BW="<<m_MaxBW;
		m_Mutex.lock();
		DeleteFilters();
		//loop until closest output rate is found and list of pointers to decimate by 2 stages is generated
		while( (f > (m_MaxBW / HB51TAP_MAX) ) && (f > MIN_OUTPUT_RATE) )
		{
			if(f >= (m_MaxBW / CIC3_MAX) )		//See if can use CIC order 3
				m_pDecimatorPtrs[n++] =
						new CDownConvert::CCicN3DecimateBy2;
			else if(f >= (m_MaxBW / HB11TAP_MAX) )	//See if can use fixed 11 Tap Halfband
				m_pDecimatorPtrs[n++] =
						new CDownConvert::CHalfBand11TapDecimateBy2();
			else if(f >= (m_MaxBW / HB15TAP_MAX) )	//See if can use Halfband 15 Tap
				m_pDecimatorPtrs[n++] =
						new CDownConvert::CHalfBandDecimateBy2(HB15TAP_LENGTH, HB15TAP_H);
			else if(f >= (m_MaxBW / HB19TAP_MAX) )	//See if can use Halfband 19 Tap
				m_pDecimatorPtrs[n++] =
						new CDownConvert::CHalfBandDecimateBy2(HB19TAP_LENGTH, HB19TAP_H);
			else if(f >= (m_MaxBW / HB23TAP_MAX) )	//See if can use Halfband 23 Tap
				m_pDecimatorPtrs[n++] =
						new CDownConvert::CHalfBandDecimateBy2(HB23TAP_LENGTH, HB23TAP_H);
			else if(f >= (m_MaxBW / HB27TAP_MAX) )	//See if can use Halfband 27 Tap
				m_pDecimatorPtrs[n++] =
						new CDownConvert::CHalfBandDecimateBy2(HB27TAP_LENGTH, HB27TAP_H);
			else if(f >= (m_MaxBW / HB31TAP_MAX) )	//See if can use Halfband 31 Tap
				m_pDecimatorPtrs[n++] =
						new CDownConvert::CHalfBandDecimateBy2(HB31TAP_LENGTH, HB31TAP_H);
			else if(f >= (m_MaxBW / HB35TAP_MAX) )	//See if can use Halfband 35 Tap
				m_pDecimatorPtrs[n++] =
						new CDownConvert::CHalfBandDecimateBy2(HB35TAP_LENGTH, HB35TAP_H);
			else if(f >= (m_MaxBW / HB39TAP_MAX) )	//See if can use Halfband 39 Tap
				m_pDecimatorPtrs[n++] =
						new CDownConvert::CHalfBandDecimateBy2(HB39TAP_LENGTH, HB39TAP_H);
			else if(f >= (m_MaxBW / HB43TAP_MAX) )	//See if can use Halfband 43 Tap
				m_pDecimatorPtrs[n++] =
						new CDownConvert::CHalfBandDecimateBy2(HB43TAP_LENGTH, HB43TAP_H);
			else if(f >= (m_MaxBW / HB47TAP_MAX) )	//See if can use Halfband 47 Tap
				m_pDecimatorPtrs[n++] =
						new CDownConvert::CHalfBandDecimateBy2(HB47TAP_LENGTH, HB47TAP_H);
			else if(f >= (m_MaxBW / HB51TAP_MAX) )	//See if can use Halfband 51 Tap
				m_pDecimatorPtrs[n++] =
						new CDownConvert::CHalfBandDecimateBy2(HB51TAP_LENGTH, HB51TAP_H);
			f /= 2.0;
		}
		m_Mutex.unlock();
		m_OutputRate = f;
		SetFrequency(m_NcoFreq);
//qDebug()<<"Filters "<<n<<" fout= "<<m_OutputRate;
	}
	return m_OutputRate;
}

//////////////////////////////////////////////////////////////////////
// Processes 'InLength' I/Q samples of 'pInData' buffer
// and places in 'pOutData' buffer.
// Returns number of samples available in output buffer.
// Make sure number of  input samples is large enough to have enough
// output samples to process in following stages since decimation
// process reduces the number of output samples per block.
// Also InLength must be a multiple of 2^N where N is the maximum
// decimation by 2 stages expected.
// ~50nSec/sample at decimation by 128
//////////////////////////////////////////////////////////////////////
int CDownConvert::ProcessData(int InLength, TYPECPX* pInData, TYPECPX* pOutData)
{
int i,j;
TYPECPX dtmp;
TYPECPX Osc;

//StartPerformance();

#if (NCO_VCASM || NCO_GCCASM)
double	dPhaseAcc = (double)m_NcoTime;
double	dASMCos   = 0.0;
double	dASMSin   = 0.0;
double*	pdCosAns  = &dASMCos;
double*	pdSinAns  = &dASMSin;
#endif

//263uS using sin/cos or 70uS using quadrature osc or 200uS using _asm
	for(i=0; i<InLength; i++)
	{
		dtmp = pInData[i];
#if NCO_LIB
		Osc.re = cos(m_NcoTime);
		Osc.im = sin(m_NcoTime);
		m_NcoTime += m_NcoInc;
#elif NCO_OSC
		TYPEREAL OscGn;
		Osc.re = m_Osc1.re * m_OscCos - m_Osc1.im * m_OscSin;
		Osc.im = m_Osc1.im * m_OscCos + m_Osc1.re * m_OscSin;
		OscGn = 1.95 - (m_Osc1.re*m_Osc1.re + m_Osc1.im*m_Osc1.im);
		m_Osc1.re = OscGn * Osc.re;
		m_Osc1.im = OscGn * Osc.im;
#elif NCO_VCASM
		_asm
		{
			fld QWORD PTR [dPhaseAcc]
			fsincos
			mov ebx,[pdCosAns]		;	get the pointer into ebx
			fstp QWORD PTR [ebx]	;	store the result through the pointer
			mov ebx,[pdSinAns]
			fstp QWORD PTR [ebx]
		}
		dPhaseAcc += m_NcoInc;
		Osc.re = dASMCos;
		Osc.im = dASMSin;
#elif NCO_GCCASM
		asm volatile ("fsincos" : "=%&t" (dASMCos), "=%&u" (dASMSin) : "0" (dPhaseAcc));
		dPhaseAcc += m_NcoInc;
		Osc.re = dASMCos;
		Osc.im = dASMSin;
#endif

		//Cpx multiply by shift frequency
		pInData[i].re = ((dtmp.re * Osc.re) - (dtmp.im * Osc.im));
		pInData[i].im = ((dtmp.re * Osc.im) + (dtmp.im * Osc.re));
	}
#if (NCO_VCASM || NCO_GCCASM)
	m_NcoTime = dPhaseAcc;
#elif !NCO_OSC
	m_NcoTime = fmod(m_NcoTime, K_2PI);	//keep radian counter bounded
#endif

	//now perform decimation of pInData by calling decimate by 2 stages
	//until NULL pointer encountered designating end of chain
	int n = InLength;
	j = 0;
	m_Mutex.lock();
	while(m_pDecimatorPtrs[j])
	{
		n = m_pDecimatorPtrs[j++]->DecBy2(n, pInData, pInData);
//if(1==j)
//g_pTestBench->DisplayData(n, (TYPECPX*)pInData, 615385/2.0);
	}
	m_Mutex.unlock();
	for(i=0; i<n; i++)
		pOutData[i] = pInData[i];
//StopPerformance(InLength);
	return n;
}

// *&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*

//////////////////////////////////////////////////////////////////////
//Decimate by 2 Halfband filter class implementation
//////////////////////////////////////////////////////////////////////
CDownConvert::CHalfBandDecimateBy2::CHalfBandDecimateBy2(int len,const TYPEREAL* pCoef )
	: m_FirLength(len), m_pCoef(pCoef)
{
	//create buffer for FIR implementation
	m_pHBFirBuf = new TYPECPX[MAX_HALF_BAND_BUFSIZE];
	TYPECPX CPXZERO = {0.0,0.0};
	for(int i=0; i<MAX_HALF_BAND_BUFSIZE ;i++)
		m_pHBFirBuf[i] = CPXZERO;
}

//////////////////////////////////////////////////////////////////////
// Half band filter and decimate by 2 function.
// Two restrictions on this routine:
// InLength must be larger or equal to the Number of Halfband Taps
// InLength must be an even number  ~37nS
//////////////////////////////////////////////////////////////////////
int CDownConvert::CHalfBandDecimateBy2::DecBy2(int InLength, TYPECPX* pInData, TYPECPX* pOutData)
{
int i;
int j;
int numoutsamples = 0;
	if(InLength<m_FirLength)	//safety net to make sure InLength is large enough to process
		return InLength/2;
//StartPerformance();
	//copy input samples into buffer starting at position m_FirLength-1
	for(i=0,j = m_FirLength - 1; i<InLength; i++)
		m_pHBFirBuf[j++] = pInData[i];
	//perform decimation FIR filter on even samples
	for(i=0; i<InLength; i+=2)
	{
		TYPECPX acc;
		acc.re = ( m_pHBFirBuf[i].re * m_pCoef[0] );
		acc.im = ( m_pHBFirBuf[i].im * m_pCoef[0] );
		for(j=2; j<m_FirLength; j+=2)	//only use even coefficients since odd are zero(except center point)
		{
			acc.re += ( m_pHBFirBuf[i+j].re * m_pCoef[j] );
			acc.im += ( m_pHBFirBuf[i+j].im * m_pCoef[j] );
		}
		//now multiply the center coefficient
		acc.re += ( m_pHBFirBuf[i+(m_FirLength-1)/2].re * m_pCoef[(m_FirLength-1)/2] );
		acc.im += ( m_pHBFirBuf[i+(m_FirLength-1)/2].im * m_pCoef[(m_FirLength-1)/2] );
		pOutData[numoutsamples++] = acc;	//put output buffer

	}
	//need to copy last m_FirLength - 1 input samples in buffer to beginning of buffer
	// for FIR wrap around management
	for(i=0,j = InLength-m_FirLength+1; i<m_FirLength - 1; i++)
		m_pHBFirBuf[i] = pInData[j++];
//StopPerformance(InLength);
	return numoutsamples;
}

// *&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*

//////////////////////////////////////////////////////////////////////
//Decimate by 2 Fixed 11 Tap Halfband filter class implementation
// Loop unrolled for speed ~9nS /samp
//////////////////////////////////////////////////////////////////////
CDownConvert::CHalfBand11TapDecimateBy2::CHalfBand11TapDecimateBy2()
{
	//preload only the taps that are used since evey other one is zero
	//except center tap 5
	H0 = HB11TAP_H[0]; H2 = HB11TAP_H[2]; H4 = HB11TAP_H[4];
	H5 = HB11TAP_H[5];
	H6 = HB11TAP_H[6]; H8 = HB11TAP_H[8]; H10 = HB11TAP_H[10];
	TYPECPX CPXZERO = {0.0,0.0};
	d0 = CPXZERO; d1 = CPXZERO;	d2 = CPXZERO; d3 = CPXZERO;
	d4 = CPXZERO; d5 = CPXZERO;	d6 = CPXZERO; d7 = CPXZERO;
	d8 = CPXZERO; d9 = CPXZERO;
}

//////////////////////////////////////////////////////////////////////
//Decimate by 2 Fixed 11 Tap Halfband filter class implementation
// Two restrictions on this routine:
// InLength must be larger or equal to the Number of Halfband Taps(11)
// InLength must be an even number
// Loop unrolled for speed ~15nS/samp
//////////////////////////////////////////////////////////////////////
int CDownConvert::CHalfBand11TapDecimateBy2::DecBy2(int InLength, TYPECPX* pInData, TYPECPX* pOutData)
{
//StartPerformance();
	//first calculate beginning 10 samples using previous samples in delay buffer
	TYPECPX tmpout[9];	//use temp buffer so outbuf can be same as inbuf
	tmpout[0].re = H0*d0.re + H2*d2.re + H4*d4.re + H5*d5.re + H6*d6.re + H8*d8.re
					 + H10*pInData[0].re;
	tmpout[0].im = H0*d0.im + H2*d2.im + H4*d4.im + H5*d5.im + H6*d6.im + H8*d8.im
					 + H10*pInData[0].im;

	tmpout[1].re = H0*d2.re + H2*d4.re + H4*d6.re + H5*d7.re + H6*d8.re
					 + H8*pInData[0].re + H10*pInData[2].re;
	tmpout[1].im = H0*d2.im + H2*d4.im + H4*d6.im + H5*d7.im + H6*d8.im
					 + H8*pInData[0].im + H10*pInData[2].im;

	tmpout[2].re = H0*d4.re + H2*d6.re + H4*d8.re + H5*d9.re
					 + H6*pInData[0].re + H8*pInData[2].re + H10*pInData[4].re;
	tmpout[2].im = H0*d4.im + H2*d6.im + H4*d8.im + H5*d9.im
					 + H6*pInData[0].im + H8*pInData[2].im + H10*pInData[4].im;

	tmpout[3].re = H0*d6.re + H2*d8.re + H4*pInData[0].re + H5*pInData[1].re
					 + H6*pInData[2].re + H8*pInData[4].re + H10*pInData[6].re;
	tmpout[3].im = H0*d6.im + H2*d8.im + H4*pInData[0].im + H5*pInData[1].im
					 + H6*pInData[2].im + H8*pInData[4].im + H10*pInData[6].im;

	tmpout[4].re = H0*d8.re + H2*pInData[0].re + H4*pInData[2].re + H5*pInData[3].re
					 + H6*pInData[4].re + H8*pInData[6].re + H10*pInData[8].re;
	tmpout[4].im = H0*d8.im + H2*pInData[0].im + H4*pInData[2].im + H5*pInData[3].im
					 + H6*pInData[4].im + H8*pInData[6].im + H10*pInData[8].im;

	tmpout[5].re = H0*pInData[0].re + H2*pInData[2].re + H4*pInData[4].re + H5*pInData[5].re
					 + H6*pInData[6].re + H8*pInData[8].re + H10*pInData[10].re;
	tmpout[5].im = H0*pInData[0].im + H2*pInData[2].im + H4*pInData[4].im + H5*pInData[5].im
					 + H6*pInData[6].im + H8*pInData[8].im + H10*pInData[10].im;

	tmpout[6].re = H0*pInData[2].re + H2*pInData[4].re + H4*pInData[6].re + H5*pInData[7].re
					 + H6*pInData[8].re + H8*pInData[10].re + H10*pInData[12].re;
	tmpout[6].im = H0*pInData[2].im + H2*pInData[4].im + H4*pInData[6].im + H5*pInData[7].im
					 + H6*pInData[8].im + H8*pInData[10].im + H10*pInData[12].im;

	tmpout[7].re = H0*pInData[4].re + H2*pInData[6].re + H4*pInData[8].re + H5*pInData[9].re
					 + H6*pInData[10].re + H8*pInData[12].re + H10*pInData[14].re;
	tmpout[7].im = H0*pInData[4].im + H2*pInData[6].im + H4*pInData[8].im + H5*pInData[9].im
					 + H6*pInData[10].im + H8*pInData[12].im + H10*pInData[14].im;

	tmpout[8].re = H0*pInData[6].re + H2*pInData[8].re + H4*pInData[10].re + H5*pInData[11].re
					 + H6*pInData[12].re + H8*pInData[14].re + H10*pInData[16].re;
	tmpout[8].im = H0*pInData[6].im + H2*pInData[8].im + H4*pInData[10].im + H5*pInData[11].im
					 + H6*pInData[12].im + H8*pInData[14].im + H10*pInData[16].im;

	//now loop through remaining input samples
	TYPECPX* pIn = &pInData[8];
	TYPECPX* pOut = &pOutData[9];
	for(int i=0; i<(InLength-11-6 )/2; i++)
	{
		(*pOut).re   = H0*pIn[0].re + H2*pIn[2].re + H4*pIn[4].re + H5*pIn[5].re
					  + H6*pIn[6].re + H8*pIn[8].re + H10*pIn[10].re;
		(*pOut++).im = H0*pIn[0].im + H2*pIn[2].im + H4*pIn[4].im + H5*pIn[5].im
					  + H6*pIn[6].im + H8*pIn[8].im + H10*pIn[10].im;
		pIn += 2;
	}
	//copy first outputs back into output array so outbuf can be same as inbuf
	pOutData[0] = tmpout[0]; pOutData[1] = tmpout[1];
	pOutData[2] = tmpout[2]; pOutData[3] = tmpout[3];
	pOutData[4] = tmpout[4]; pOutData[5] = tmpout[5];
	pOutData[6] = tmpout[6]; pOutData[7] = tmpout[7];
	pOutData[8] = tmpout[8];

	//copy last 10 input samples into delay buffer for next time
	pIn = &pInData[InLength-1];
	d9 = *pIn--; d8 = *pIn--; d7 = *pIn--;
	d6 = *pIn--; d5 = *pIn--; d4 = *pIn--;
	d3 = *pIn--; d2 = *pIn--; d1 = *pIn--; d0 = *pIn;
//StopPerformance(InLength);
	return InLength/2;
}

// *&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*&*

//////////////////////////////////////////////////////////////////////
//Decimate by 2 CIC 3 stage
// -80dB alias rejection up to Fs * (.5 - .4985)
//////////////////////////////////////////////////////////////////////
CDownConvert::CCicN3DecimateBy2::CCicN3DecimateBy2()
{
	m_Xodd.re = 0.0; m_Xodd.im = 0.0;
	m_Xeven.re = 0.0; m_Xeven.im = 0.0;
}

//////////////////////////////////////////////////////////////////////
//Function performs decimate by 2 using polyphase decompostion
// implemetation of a CIC N=3 filter.
// InLength must be an even number
//returns number of output samples processed
// 6nS/sample
//////////////////////////////////////////////////////////////////////
int CDownConvert::CCicN3DecimateBy2::DecBy2(int InLength, TYPECPX* pInData, TYPECPX* pOutData)
{
int i,j;
TYPECPX even,odd;
//StartPerformance();
	for(i=0,j=0; i<InLength; i+=2,j++)
	{	//mag gn=8
		even = pInData[i];
		odd = pInData[i+1];
		pOutData[j].re = .125*( odd.re + m_Xeven.re + 3.0*(m_Xodd.re + even.re) );
		pOutData[j].im = .125*( odd.im + m_Xeven.im + 3.0*(m_Xodd.im + even.im) );
		m_Xodd = odd;
		m_Xeven = even;
	}
//StopPerformance(InLength);
	return j;
}
