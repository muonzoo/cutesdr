//////////////////////////////////////////////////////////////////////
// noiseproc.cpp: implementation of the CNoiseProc class.
//
//  This class implements various noise processing functions.
//   Impulse blanker, Noise Reduction, and Notch
//
// History:
//	2011-01-06  Initial creation MSW
//	2011-03-27  Initial release(not implemented yet)
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

#include "dsp/noiseproc.h"
#include "gui/testbench.h"
#include "interface/perform.h"
#include <QDebug>

//////////////////////////////////////////////////////////////////////
// Local Defines
//////////////////////////////////////////////////////////////////////
#define MAX_WIDTH 4096		//abt 1 mSec at 2MHz
#define MAX_DELAY 4096		//abt 1 mSec at 2MHz
#define MAX_AVE 32768		//abt 10 mSec at 2MHz

#define MAGAVE_TIME 0.005

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CNoiseProc::CNoiseProc()
{
	m_DelayBuf = new TYPECPX[MAX_DELAY];
	m_MagBuf = new TYPEREAL[MAX_AVE];
	m_TestBenchDataBuf = new TYPECPX[4096];
	SetupBlanker(false, 50.0, 2.0, 1000.0);
}

CNoiseProc::~CNoiseProc()
{
	if(m_DelayBuf)
		delete m_DelayBuf;
	if(m_MagBuf)
		delete m_MagBuf;
	if(m_TestBenchDataBuf)
		delete m_TestBenchDataBuf;
}

void CNoiseProc::SetupBlanker(bool On, TYPEREAL Threshold, TYPEREAL Width, TYPEREAL SampleRate)
{
	if( (Threshold==m_Threshold) &&
		(Width==m_Width) &&
		(m_On == On) &&
		(SampleRate==SampleRate) )
	{
		return;
	}
	m_Mutex.lock();
	m_On = On;
	m_Threshold = Threshold;
	m_Width = Width;
	m_SampleRate = SampleRate;

	m_WidthSamples = Width * 1e-6 * m_SampleRate;
	if(m_WidthSamples < 1)
		m_WidthSamples = 1;
	else if(m_WidthSamples>MAX_WIDTH)
		m_WidthSamples = MAX_WIDTH;

	m_MagSamples = MAGAVE_TIME*m_SampleRate;

	m_Ratio = .005*(m_Threshold)*(TYPEREAL)m_MagSamples;

	m_DelaySamples = m_WidthSamples/2;

//	qDebug()<<"m_DelaySamples="<<m_DelaySamples << m_Ratio;

	m_Dptr = 0;
	m_Mptr = 0;
	m_BlankCounter = 0;
	m_MagAveSum = 0.0;
	for(int i=0; i<MAX_DELAY ; i++)
	{
		m_DelayBuf[i].re = 0.0;
		m_DelayBuf[i].im = 0.0;
	}
	for(int i=0; i<MAX_AVE ; i++)
		m_MagBuf[i] = 0.0;

	m_Mutex.unlock();
}

void CNoiseProc::ProcessBlanker(int InLength, TYPECPX* pInData, TYPECPX* pOutData)
{
TYPECPX newsamp;
TYPECPX oldest;
	if(!m_On)
	{
		g_pTestBench->DisplayData(InLength, pOutData, m_SampleRate,PROFILE_7);
		return;
	}
	m_Mutex.lock();
//StartPerformance();
	for(int i=0; i<InLength; i++)
	{
		newsamp = pInData[i];

		//calculate magnitude peak magnitude
		TYPEREAL mre = fabs(newsamp.re);
		TYPEREAL mim = fabs(newsamp.im);
		TYPEREAL mag = (mre>mim) ? mre : mim;

//m_TestBenchDataBuf[i].im = mag;
		//calc moving average of "m_MagSamples"
		m_MagAveSum -= m_MagBuf[m_Mptr];	//subtract oldest sample
		m_MagAveSum += mag;					//add new sample
		m_MagBuf[m_Mptr++] = mag;			//stick in buffer
		if(m_Mptr > m_MagSamples)
			m_Mptr = 0;

		//pull out oldest sample from delay buffer and put in new one
		oldest = m_DelayBuf[m_Dptr];
		m_DelayBuf[m_Dptr++] = newsamp;
		if(m_Dptr > m_DelaySamples)
			m_Dptr = 0;

		if( mag*m_Ratio > m_MagAveSum )
		{
			m_BlankCounter = m_WidthSamples;
		}
m_TestBenchDataBuf[i].im = oldest.re + oldest.im;
		if(m_BlankCounter)
		{
			m_BlankCounter--;
m_TestBenchDataBuf[i].re = 0.0;
			pOutData[i].re = 0.0;
			pOutData[i].im = 0.0;
		}
		else
		{
m_TestBenchDataBuf[i].re = m_MagAveSum/m_Ratio;
			pOutData[i] = oldest;
		}
	}
	m_Mutex.unlock();
//StopPerformance(InLength);
g_pTestBench->DisplayData(InLength, m_TestBenchDataBuf, m_SampleRate,PROFILE_7);
}
