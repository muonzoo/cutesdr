// smeter.cpp: implementation of the CS<eter class.
//
//  This class takes magnitude data and detemines peak and average power
// for use in S Meter display
//
// History:
//	2010-12-22  Initial creation MSW
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
#include "smeter.h"
#include <QDebug>

#define  ATTACK_TIMECONST .01	//attack time in seconds
#define  DECAY_TIMECONST .5		//decay time in seconds

#define SMETER_CALIBRATION 5.0	//S-Meter calibration offset added to make reading absolute dBm

#define MAX_PWR (32767.0*32767.0)

CSMeter::CSMeter()
{
	m_PeakMag = 0;
	m_SampleRate = 1.0;
	m_AttackAlpha = 1.0;
	m_DecayAlpha = 1.0;
	m_AttackAve = -120.0;
	m_DecayAve = -120.0;
}

////////////////////////////////////////////////////////////////////////////////////
// Process magnitude data and set S-Meter power variables
////////////////////////////////////////////////////////////////////////////////////
void CSMeter::ProcessData(int length, TYPECPX* pInData, TYPEREAL SampleRate)
{
	if(SampleRate != m_SampleRate)
	{	//need to recalculate any values dependent on sample rate
		m_SampleRate = SampleRate;
		m_AttackAlpha = (1.0-exp(-1.0/(m_SampleRate*ATTACK_TIMECONST)) );
		m_DecayAlpha = (1.0-exp(-1.0/(m_SampleRate*DECAY_TIMECONST)) );
//qDebug()<<"SMeter vals "<<m_AttackAlpha << m_DecayAlpha << SampleRate;
	}
	for(int i=0; i<length; i++)
	{
		//calculate instantaeous power magnitude of pInData which is I*I + Q*Q
		TYPECPX in = pInData[i];
		//convert I/Q magnitude to dB power
		TYPEREAL mag = 10.0*log10((in.re*in.re+in.im*in.im)/ MAX_PWR + 1e-50);
		//calculate attack and decay average
		m_AttackAve = (1.0-m_AttackAlpha)*m_AttackAve + m_AttackAlpha*mag;
		m_DecayAve = (1.0-m_DecayAlpha)*m_DecayAve + m_DecayAlpha*mag;
		if(m_AttackAve>m_DecayAve)
		{	//if attack average is greater then must be increasing signal
			m_AverageMag = m_AttackAve;	//use attack average value
			m_DecayAve = m_AttackAve;	//force decay average to attack average
		}
		else
		{	//is decreasing strength so use decay average
			m_AverageMag = m_DecayAve;	//use decay average value
		}
		if(mag > m_PeakMag)
			m_PeakMag = mag;		//save new peak (reset when read )
	}

}


////////////////////////////////////////////////////////////////////////////////////
// returns Peak S-Meter power variable
////////////////////////////////////////////////////////////////////////////////////
TYPEREAL CSMeter::GetPeak()
{
	TYPEREAL x = m_PeakMag;
	m_PeakMag = 0;
	return x+SMETER_CALIBRATION;
}

////////////////////////////////////////////////////////////////////////////////////
// returns Average S-Meter power variable with attack and decay time averaging
////////////////////////////////////////////////////////////////////////////////////
TYPEREAL CSMeter::GetAve()
{
	return m_AverageMag + SMETER_CALIBRATION;
}

