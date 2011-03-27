/////////////////////////////////////////////////////////////////////
// demodulator.cpp: implementation of the Cdemodulator class.
//
//	This class implements the demodulation DSP functionality to take
//raw I/Q data from the radio, shift to baseband, decimate, demodulate,
//perform AGC, and send the audio to the sound card.
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
/////////////////////////////////////////////////////////////////////
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
#include "dsp/demodulator.h"
#include "gui/testbench.h"
#include <QDebug>

//////////////////////////////////////////////////////////////////
//	Constructor/Destructor
//////////////////////////////////////////////////////////////////
CDemodulator::CDemodulator()
{
	m_DesiredMaxOutputBandwidth = 48000.0;
	m_OutputRate = 48000.0;
	m_pDemodInBuf = new TYPECPX[MAX_INBUFSIZE];
	m_pDemodTmpBuf = new TYPECPX[MAX_INBUFSIZE];
	m_InBufPos = 0;
	m_InBufLimit = 1000;
	m_DemodMode = -1;
	m_pAmDemod = NULL;
	m_pSamDemod = NULL;
	m_pSsbDemod = NULL;
	SetDemodFreq(0.0);
}

CDemodulator::~CDemodulator()
{
	DeleteAllDemods();
	if(m_pDemodInBuf)
		delete m_pDemodInBuf;
	if(m_pDemodTmpBuf)
		delete m_pDemodTmpBuf;
}

//////////////////////////////////////////////////////////////////
//	Deletes all demod objects
//////////////////////////////////////////////////////////////////
void CDemodulator::DeleteAllDemods()
{
	if(m_pAmDemod)
		delete m_pAmDemod;
	if(m_pSamDemod)
		delete m_pSamDemod;
	if(m_pFmDemod)
		delete m_pFmDemod;
	if(m_pSsbDemod)
		delete m_pSsbDemod;
	m_pAmDemod = NULL;
	m_pSamDemod = NULL;
	m_pFmDemod = NULL;
	m_pSsbDemod = NULL;
}


//////////////////////////////////////////////////////////////////
//	Called to set/change the demodulator input sample rate
//////////////////////////////////////////////////////////////////
void CDemodulator::SetInputSampleRate(TYPEREAL InputRate)
{
	if(m_InputRate != InputRate)
	{
		m_InputRate = InputRate;
		m_OutputRate = m_DownConvert.SetDataRate(m_InputRate, m_DesiredMaxOutputBandwidth);
	}
}

//////////////////////////////////////////////////////////////////
//	Called to set/change the active Demod object
//or if a demod parameter or filter parameter changes
//////////////////////////////////////////////////////////////////
void CDemodulator::SetDemod(int Mode, tDemodInfo CurrentDemodInfo)
{
	m_Mutex.lock();
	m_DemodInfo = CurrentDemodInfo;
	if(m_DemodMode != Mode)	//do only if changes
	{
		DeleteAllDemods();		//remove current demod object
		m_DemodMode = Mode;
		//create decimation chain and get output sample rate
		if((DEMOD_LSB == m_DemodMode) || (DEMOD_CWL == m_DemodMode) )
			m_DesiredMaxOutputBandwidth = -m_DemodInfo.LowCutmin;
		else
			m_DesiredMaxOutputBandwidth = m_DemodInfo.HiCutmax;
		m_OutputRate = m_DownConvert.SetDataRate(m_InputRate, m_DesiredMaxOutputBandwidth);
		//now create correct demodulator
		switch(m_DemodMode)
		{
			case DEMOD_AM:
				m_pAmDemod = new CAmDemod(m_OutputRate);
				break;
			case DEMOD_SAM:
				m_pSamDemod = new CSamDemod(m_OutputRate);
				break;
			case DEMOD_FM:
				m_pFmDemod = new CFmDemod(m_OutputRate);
				break;
			case DEMOD_USB:
			case DEMOD_LSB:
			case DEMOD_CWU:
			case DEMOD_CWL:
				m_pSsbDemod = new CSsbDemod();
				break;
		}
	}
	m_CW_Offset = m_DemodInfo.Offset;
	m_DownConvert.SetCwOffset(m_CW_Offset);
	m_FastFIR.SetupParameters(m_DemodInfo.LowCut, m_DemodInfo.HiCut,m_CW_Offset,m_OutputRate);
	//set input buffer limit so that decimated output is abt 10mSec or more of data
	m_InBufLimit = (m_OutputRate/100.0) * m_InputRate/m_OutputRate;	//process abt .01sec of output samples at a time
	m_InBufLimit &= 0xFFFFFF00;	//keep modulo 256 since decimation is only in power of 2
	m_Agc.SetParameters(m_DemodInfo.AgcOn, m_DemodInfo.AgcHangOn, m_DemodInfo.AgcThresh,
						m_DemodInfo.AgcManualGain, m_DemodInfo.AgcSlope, m_DemodInfo.AgcDecay, m_OutputRate);
	if(	m_pFmDemod != NULL)
		m_pFmDemod->SetSquelch(m_DemodInfo.SquelchValue);
	if(m_pAmDemod != NULL)
		m_pAmDemod->SetBandwidth( (m_DemodInfo.HiCut-m_DemodInfo.LowCut)/2.0);

	m_Mutex.unlock();
//qDebug()<<"m_InputRate="<<m_InputRate<<" DesiredMaxOutputBandwidth=="<<m_DesiredMaxOutputBandwidth <<"OutputRate="<<m_OutputRate;
//qDebug()<<"m_InBufLimit="<<m_InBufLimit;
}

//////////////////////////////////////////////////////////////////
//	Called with complex data from radio and performs the demodulation
// with MONO audio output
//////////////////////////////////////////////////////////////////
int CDemodulator::ProcessData(int InLength, TYPECPX* pInData, TYPEREAL* pOutData)
{
int ret = 0;
	m_Mutex.lock();
	for(int i=0; i<InLength; i++)
	{	//place in demod buffer
		m_pDemodInBuf[m_InBufPos++] = pInData[i];
		if(m_InBufPos >= m_InBufLimit)
		{	//when have enough samples, call demod routine sequence

			//perform baseband tuning and decimation
			int n = m_DownConvert.ProcessData(m_InBufPos, m_pDemodInBuf, m_pDemodInBuf);
			g_pTestBench->DisplayData(n, m_pDemodInBuf, m_OutputRate,PROFILE_1);


			//perform main bandpass filtering
			n = m_FastFIR.ProcessData(n, m_pDemodInBuf, m_pDemodTmpBuf);
			g_pTestBench->DisplayData(n, m_pDemodTmpBuf, m_OutputRate,PROFILE_2);

			//perform S-Meter processing
			m_SMeter.ProcessData(n, m_pDemodTmpBuf, m_OutputRate);

			//perform AGC
			m_Agc.ProcessData(n, m_pDemodTmpBuf, m_pDemodTmpBuf );
			g_pTestBench->DisplayData(n, m_pDemodTmpBuf, m_OutputRate, PROFILE_3);

			//perform the desired demod action
			switch(m_DemodMode)
			{
				case DEMOD_AM:
					n = m_pAmDemod->ProcessData(n, m_pDemodTmpBuf, pOutData );
					break;
				case DEMOD_SAM:
					n = m_pSamDemod->ProcessData(n, m_pDemodTmpBuf, pOutData );
					break;
				case DEMOD_FM:
					n = m_pFmDemod->ProcessData(n, m_DemodInfo.HiCut, m_pDemodTmpBuf, pOutData );
					break;
				case DEMOD_USB:
				case DEMOD_LSB:
				case DEMOD_CWU:
				case DEMOD_CWL:
					n = m_pSsbDemod->ProcessData(n, m_pDemodTmpBuf, pOutData);
					break;
			}
			g_pTestBench->DisplayData(n, pOutData, m_OutputRate,PROFILE_4);
			m_InBufPos = 0;
			ret += n;
		}
	}
	m_Mutex.unlock();
	return ret;
}

//////////////////////////////////////////////////////////////////
//	Called with complex data from radio and performs the demodulation
// with STEREO audio output
//////////////////////////////////////////////////////////////////
int CDemodulator::ProcessData(int InLength, TYPECPX* pInData, TYPECPX* pOutData)
{
int ret = 0;
	m_Mutex.lock();
	for(int i=0; i<InLength; i++)
	{	//place in demod buffer
		m_pDemodInBuf[m_InBufPos++] = pInData[i];
		if(m_InBufPos >= m_InBufLimit)
		{	//when have enough samples, call demod routine sequence

			//perform baseband tuning and decimation
			int n = m_DownConvert.ProcessData(m_InBufPos, m_pDemodInBuf, m_pDemodInBuf);
			g_pTestBench->DisplayData(n, m_pDemodInBuf, m_OutputRate,PROFILE_1);


			//perform main bandpass filtering
			n = m_FastFIR.ProcessData(n, m_pDemodInBuf, m_pDemodTmpBuf);
			g_pTestBench->DisplayData(n, m_pDemodTmpBuf, m_OutputRate,PROFILE_2);

			//perform S-Meter processing
			m_SMeter.ProcessData(n, m_pDemodTmpBuf, m_OutputRate);

			//perform AGC
			m_Agc.ProcessData(n, m_pDemodTmpBuf, m_pDemodTmpBuf );
			g_pTestBench->DisplayData(n, m_pDemodTmpBuf, m_OutputRate, PROFILE_3);

			//perform the desired demod action
			switch(m_DemodMode)
			{
				case DEMOD_AM:
					n = m_pAmDemod->ProcessData(n, m_pDemodTmpBuf, pOutData );
					break;
				case DEMOD_SAM:
					n = m_pSamDemod->ProcessData(n, m_pDemodTmpBuf, pOutData );
					break;
				case DEMOD_FM:
					n = m_pFmDemod->ProcessData(n, m_DemodInfo.HiCut, m_pDemodTmpBuf, pOutData );
					break;
				case DEMOD_USB:
				case DEMOD_LSB:
				case DEMOD_CWU:
				case DEMOD_CWL:
					n = m_pSsbDemod->ProcessData(n, m_pDemodTmpBuf, pOutData);
					break;
			}
		g_pTestBench->DisplayData(n, pOutData, m_OutputRate,PROFILE_4);
			m_InBufPos = 0;
			ret += n;
		}
	}
	m_Mutex.unlock();
	return ret;
}
