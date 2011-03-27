// samdemod.cpp: implementation of the CSamDemod class.
//
//  This class takes I/Q baseband data and performs
// Synchronous AM demodulation
//
// History:
//	2010-09-22  Initial creation MSW
//	2011-03-27  Initial release
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
#include "samdemod.h"
#include "gui/testbench.h"
#include "dsp/datatypes.h"
#include "dsp/filtercoef.h"
#include <QDebug>

#define DC_ALPHA 0.99	//ALPHA for DC removal filter ~20Hz Fcut with 15625Hz Sample Rate

#define PLL_BW 100.0		//natural frequency ~ loop bandwidth
#define PLL_ZETA .707		//PLL Loop damping factor
#define PLL_LIMIT 1000.0	//+- frequency limit in Hz

/////////////////////////////////////////////////////////////////////////////////
//	Construct SAM demod object
/////////////////////////////////////////////////////////////////////////////////
CSamDemod::CSamDemod(TYPEREAL samplerate) : m_SampleRate(samplerate)
{
	m_y1 = 0.0;
	m_z1 = 0.0;

	m_NcoPhase = 0.0;
	m_NcoFreq = 0.0;
	TYPEREAL norm = K_2PI/m_SampleRate;
	m_NcoLLimit = -PLL_LIMIT * norm;		//clamp SAM NCO to +-1KHz
	m_NcoHLimit = PLL_LIMIT * norm;
	m_PllAlpha = 2.0*PLL_ZETA*PLL_BW * norm;
	m_PllBeta = (m_PllAlpha * m_PllAlpha)/(4.0*PLL_ZETA*PLL_ZETA);

	//create complex lowpass filter pair with LP cuttoff of 5000Hz and transition width of 1000Hz
	// 40dB stop band attenuation
	m_Fir.InitLPFilter(1.0, 40.0, 4500, 5500,m_SampleRate);
	//apply transform to shift the LP filter 5000Hz so now the filter is
	// a 0 to 10000Hz Hilbert bandpass filter with 90 degree phase shift
	m_Fir.GenerateHBFilter(5000.0);
}

/////////////////////////////////////////////////////////////////////////////////
//	Process SAM demod MONO version
/////////////////////////////////////////////////////////////////////////////////
int CSamDemod::ProcessData(int InLength, TYPECPX* pInData, TYPEREAL* pOutData)
{
TYPECPX tmp;
	for(int i=0; i<InLength; i++)
	{
		TYPEREAL Sin = -sin(m_NcoPhase);
		TYPEREAL Cos = cos(m_NcoPhase);
		//complex multiply input sample by NCO's  -sin and cos
		tmp.re = Cos * pInData[i].re - Sin * pInData[i].im;
		tmp.im = Cos * pInData[i].im + Sin * pInData[i].re;
		//find current sample phase after being shifted by NCO frequency
		TYPEREAL phzerror = atan2(tmp.im, tmp.re);

//TYPEREAL test = phzerror*100.0;
//g_pTestBench->DisplayData(1, &test, m_SampleRate,PROFILE_6);

		m_NcoFreq += (m_PllBeta * phzerror);		//  radians per sampletime
		//clamp NCO frequency so doesn't drift out of lock range
		if(m_NcoFreq > m_NcoHLimit)
			m_NcoFreq = m_NcoHLimit;
		else if(m_NcoFreq < m_NcoLLimit)
			m_NcoFreq = m_NcoLLimit;
		//update NCO phase with new value
		m_NcoPhase += (m_NcoFreq + m_PllAlpha * phzerror);
		//High pass filter(DC removal) with IIR filter
		// H(z) = (1 - z^-1)/(1 - ALPHA*z^-1)
		TYPEREAL z0 = tmp.re + (m_z1 * DC_ALPHA);	//just use real output for mono
		pOutData[i] = (z0 - m_z1);
		m_z1 = z0;
	}
	m_NcoPhase = fmod(m_NcoPhase, K_2PI);	//keep radian counter bounded
	return InLength;
}

/////////////////////////////////////////////////////////////////////////////////
//	Process SAM demod STEREO version
/////////////////////////////////////////////////////////////////////////////////
int CSamDemod::ProcessData(int InLength, TYPECPX* pInData, TYPECPX* pOutData)
{
TYPECPX tmp;
	for(int i=0; i<InLength; i++)
	{
		TYPEREAL Sin = sin(m_NcoPhase);
		TYPEREAL Cos = cos(m_NcoPhase);
		//complex multiply input sample by NCO's  sin and cos
		tmp.re = Cos * pInData[i].re - Sin * pInData[i].im;
		tmp.im = Cos * pInData[i].im + Sin * pInData[i].re;
		//find current sample phase error after being shifted by NCO frequency
		TYPEREAL phzerror = -atan2(tmp.im, tmp.re);

//TYPEREAL test = phzerror*100.0;
//g_pTestBench->DisplayData(1, &test, m_SampleRate,PROFILE_6);

		m_NcoFreq += (m_PllBeta * phzerror);		//  radians per sampletime
		//clamp NCO frequency so doesn't drift out of lock range
		if(m_NcoFreq > m_NcoHLimit)
			m_NcoFreq = m_NcoHLimit;
		else if(m_NcoFreq < m_NcoLLimit)
			m_NcoFreq = m_NcoLLimit;
		//update NCO phase with new value
		m_NcoPhase += (m_NcoFreq + m_PllAlpha * phzerror);
		//High pass filter(DC removal) with IIR filter
		// H(z) = (1 - z^-1)/(1 - ALPHA*z^-1)
		TYPEREAL z0 = tmp.re + (m_z1 * DC_ALPHA);
		TYPEREAL y0 = tmp.im + (m_y1 * DC_ALPHA);
		pOutData[i].re = (z0 - m_z1);
		pOutData[i].im = (y0 - m_y1);
		m_y1 = y0;
		m_z1 = z0;
	}
	m_NcoPhase = fmod(m_NcoPhase, K_2PI);	//keep radian counter bounded
	//process I/Q with bandpass filter with 90deg phase shift between the I and Q filters
	m_Fir.ProcessFilter(InLength, pOutData, pOutData);
	for(int i=0; i<InLength; i++)
	{
		tmp = pOutData[i];
		pOutData[i].im = tmp.re - tmp.im;	//send upper sideband to (right)channel
		pOutData[i].re = tmp.re + tmp.im;	//send lower sideband to (left)channel
	}
	return InLength;
}
