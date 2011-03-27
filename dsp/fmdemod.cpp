// fmdemod.cpp: implementation of the CFmDemod class.
//
//  This class takes I/Q baseband data and performs
// FM demodulation and squelch
//
// History:
//	2011-01-17  Initial creation MSW
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
#include "fmdemod.h"
#include "gui/testbench.h"
#include "dsp/datatypes.h"
#include <QDebug>


#define FMPLL_RANGE 6000.0	//maximum deviation limit of PLL
#define VOICE_BANDWIDTH 3000.0

#define FMPLL_BW VOICE_BANDWIDTH*2.0	//natural frequency ~loop bandwidth
#define FMPLL_ZETA .707				//PLL Loop damping factor

#define FMDC_ALPHA 0.01	//time constant for DC removal filter

#define MAX_FMOUT 25000.0

#define SQUELCH_MAX 5000.0		//roughly the maximum noise average with no signal
#define SQUELCHAVE_TIMECONST .02
#define SQUELCH_HYSTERESIS 100.0

/////////////////////////////////////////////////////////////////////////////////
//	Construct FM demod object
/////////////////////////////////////////////////////////////////////////////////
CFmDemod::CFmDemod(TYPEREAL samplerate) : m_SampleRate(samplerate)
{
	m_FreqErrorDC = 0.0;
	m_NcoPhase = 0.0;
	m_NcoFreq = 0.0;

	TYPEREAL norm = K_2PI/m_SampleRate;	//to normalize Hz to radians

	//initialize the PLL
	m_NcoLLimit = -FMPLL_RANGE * norm;		//clamp FM PLL NCO
	m_NcoHLimit = FMPLL_RANGE * norm;
	m_PllAlpha = 2.0*FMPLL_ZETA*FMPLL_BW * norm;
	m_PllBeta = (m_PllAlpha * m_PllAlpha)/(4.0*FMPLL_ZETA*FMPLL_ZETA);

	m_OutGain = MAX_FMOUT/m_NcoHLimit;	//audio output level gain value

	//DC removal filter time constant
	m_DcAlpha = (1.0 - exp(-1.0/(m_SampleRate*FMDC_ALPHA)) );

	//initialize some noise squelch items
	m_SquelchHPFreq = VOICE_BANDWIDTH;
	m_SquelchAve = 0.0;
	m_SquelchState = true;
	m_SquelchAlpha = (1.0-exp(-1.0/(m_SampleRate*SQUELCHAVE_TIMECONST)) );

	m_LpIir.InitLP(VOICE_BANDWIDTH, 1.0,m_SampleRate);
	InitNoiseSquelch();
}


/////////////////////////////////////////////////////////////////////////////////
// Sets squelch threshold based on 'Value' which goes from 0 to 99.
/////////////////////////////////////////////////////////////////////////////////
void CFmDemod::SetSquelch(int Value)
{
	m_SquelchThreshold = (TYPEREAL)(SQUELCH_MAX - (( SQUELCH_MAX*Value)/99));
}


/////////////////////////////////////////////////////////////////////////////////
// Sets up Highpass noise filter parameters based on input filter BW
/////////////////////////////////////////////////////////////////////////////////
void CFmDemod::InitNoiseSquelch()
{
	m_HpFir.InitHPFilter(1.0, 50.0, m_SquelchHPFreq, m_SquelchHPFreq*.6, m_SampleRate);
}


/////////////////////////////////////////////////////////////////////////////////
// Performs noise squelch by reading the noise power above the voice frequencies
/////////////////////////////////////////////////////////////////////////////////
void CFmDemod::PerformNoiseSquelch(int InLength, TYPEREAL* pOutData)
{
	if(InLength>MAX_SQBUF_SIZE)
		return;
	TYPEREAL sqbuf[MAX_SQBUF_SIZE];
	//high pass filter to get the high frequency noise above the voice
	m_HpFir.ProcessFilter(InLength, pOutData, sqbuf);
//g_pTestBench->DisplayData(InLength, sqbuf, m_SampleRate,PROFILE_6);
	for(int i=0; i<InLength; i++)
	{
		TYPEREAL mag = fabs( sqbuf[i] );	//get magnitude of High pass filtered data
		// exponential filter squelch magnitude
		m_SquelchAve = (1.0-m_SquelchAlpha)*m_SquelchAve + m_SquelchAlpha*mag;
//g_pTestBench->DisplayData(1, &m_SquelchAve, m_SampleRate,PROFILE_6);
	}
	//perform squelch compare to threshold using some Hysteresis
	if(0==m_SquelchThreshold)
	{	//force squelch if zero(strong signal threshold)
		m_SquelchState = true;
	}
	else if(m_SquelchState)	//if in squelched state
	{
		if(m_SquelchAve < (m_SquelchThreshold-SQUELCH_HYSTERESIS))
			m_SquelchState = false;
	}
	else
	{
		if(m_SquelchAve >= (m_SquelchThreshold+SQUELCH_HYSTERESIS))
			m_SquelchState = true;
	}
	if(m_SquelchState)
	{	//zero output if squelched
		for(int i=0; i<InLength; i++)
			pOutData[i] = 0.0;
	}
	else
	{	//low pass filter audio if squelch is open
		m_LpIir.ProcessFilter(InLength, pOutData, pOutData);
	}
}

/////////////////////////////////////////////////////////////////////////////////
//	Process FM demod MONO version
/////////////////////////////////////////////////////////////////////////////////
int CFmDemod::ProcessData(int InLength, TYPEREAL FmBW, TYPECPX* pInData, TYPEREAL* pOutData)
{
TYPECPX tmp;
	if(m_SquelchHPFreq != FmBW)
	{	//update squelch HP filter cutoff from main filter BW
		m_SquelchHPFreq = FmBW;
		InitNoiseSquelch();
	}
	for(int i=0; i<InLength; i++)
	{
		TYPEREAL Sin = sin(m_NcoPhase);
		TYPEREAL Cos = cos(m_NcoPhase);
		//complex multiply input sample by NCO's  sin and cos
		tmp.re = Cos * pInData[i].re - Sin * pInData[i].im;
		tmp.im = Cos * pInData[i].im + Sin * pInData[i].re;
		//find current sample phase after being shifted by NCO frequency
		TYPEREAL phzerror = -atan2(tmp.im, tmp.re);

		//create new NCO frequency term
		m_NcoFreq += (m_PllBeta * phzerror);		//  radians per sampletime
		//clamp NCO frequency so doesn't get out of lock range
		if(m_NcoFreq > m_NcoHLimit)
			m_NcoFreq = m_NcoHLimit;
		else if(m_NcoFreq < m_NcoLLimit)
			m_NcoFreq = m_NcoLLimit;
		//update NCO phase with new value
		m_NcoPhase += (m_NcoFreq + m_PllAlpha * phzerror);
		//LP filter the NCO frequency term to get DC offset value
		m_FreqErrorDC = (1.0-m_DcAlpha)*m_FreqErrorDC + m_DcAlpha*m_NcoFreq;
		//subtract out DC term to get FM audio
		pOutData[i] = (m_NcoFreq-m_FreqErrorDC)*m_OutGain;
	}
	m_NcoPhase = fmod(m_NcoPhase, K_2PI);	//keep radian counter bounded
	PerformNoiseSquelch(InLength, pOutData);	//calculate squelch
	return InLength;
}

/////////////////////////////////////////////////////////////////////////////////
//	Process FM demod STEREO version
/////////////////////////////////////////////////////////////////////////////////
int CFmDemod::ProcessData(int InLength, TYPEREAL FmBW, TYPECPX* pInData, TYPECPX* pOutData)
{
TYPECPX tmp;
	if(m_SquelchHPFreq != FmBW)
	{	//update Squelch HP filter cutoff from main filter BW
		m_SquelchHPFreq = FmBW;
		InitNoiseSquelch();
	}
	for(int i=0; i<InLength; i++)
	{
		TYPEREAL Sin = sin(m_NcoPhase);
		TYPEREAL Cos = cos(m_NcoPhase);
		//complex multiply input sample by NCO's  sin and cos
		tmp.re = Cos * pInData[i].re - Sin * pInData[i].im;
		tmp.im = Cos * pInData[i].im + Sin * pInData[i].re;
		//find current sample phase after being shifted by NCO frequency
		TYPEREAL phzerror = -atan2(tmp.im, tmp.re);

		m_NcoFreq += (m_PllBeta * phzerror);		//  radians per sampletime
		//clamp NCO frequency so doesn't drift out of lock range
		if(m_NcoFreq > m_NcoHLimit)
			m_NcoFreq = m_NcoHLimit;
		else if(m_NcoFreq < m_NcoLLimit)
			m_NcoFreq = m_NcoLLimit;
		//update NCO phase with new value
		m_NcoPhase += (m_NcoFreq + m_PllAlpha * phzerror);
		//LP filter the NCO frequency term to get DC offset value
		m_FreqErrorDC = (1.0-m_DcAlpha)*m_FreqErrorDC + m_DcAlpha*m_NcoFreq;
		//subtract out DC term to get FM audio
		m_OutBuf[i] = (m_NcoFreq-m_FreqErrorDC)*m_OutGain;
	}
	m_NcoPhase = fmod(m_NcoPhase, K_2PI);	//keep radian counter bounded
	PerformNoiseSquelch(InLength, m_OutBuf);
	for(int i=0; i<InLength; i++)
	{	//copy audio stream into both output channels for stereo version
		pOutData[i].re = m_OutBuf[i];
		pOutData[i].im = m_OutBuf[i];
	}
	return InLength;
}

