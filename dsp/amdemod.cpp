// amdemod.cpp: implementation of the CAmDemod class.
//
//  This class takes I/Q baseband data and performs
// AM demodulation
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
#include "amdemod.h"
#include "gui/testbench.h"
#include "dsp/datatypes.h"
#include "dsp/filtercoef.h"
#include <QDebug>


#define DC_ALPHA 0.99	//ALPHA for DC removal filter ~20Hz Fcut with 15625Hz Sample Rate


/////////////////////////////////////////////////////////////////////////////////
//	Construct AM demod object
/////////////////////////////////////////////////////////////////////////////////
CAmDemod::CAmDemod(TYPEREAL samplerate) : m_SampleRate(samplerate)
{
	m_z1 = 0.0;
	m_Fir.InitLPFilter(1.0, 50.0, 10000, 10000*1.8, m_SampleRate);//initialize LP FIR filter
}

void CAmDemod::SetBandwidth(TYPEREAL Bandwidth)
{
	//create a LP filter with passband the same as the main filter bandwidth for post audio filtering
	m_Fir.InitLPFilter(1.0, 50.0, Bandwidth, Bandwidth*1.8, m_SampleRate);//initialize LP FIR filter
}


/////////////////////////////////////////////////////////////////////////////////
//	Process envelope AM demod MONO version
/////////////////////////////////////////////////////////////////////////////////
int CAmDemod::ProcessData(int InLength, TYPECPX* pInData, TYPEREAL* pOutData)
{
	for(int i=0; i<InLength; i++)
	{
		//calculate instantaneous power magnitude of pInData which is I*I + Q*Q
		TYPECPX in = pInData[i];
		TYPEREAL mag = sqrt(in.re*in.re+in.im*in.im);
		//High pass filter(DC removal) with IIR filter
		// H(z) = (1 - z^-1)/(1 - ALPHA*z^-1)
		TYPEREAL z0 = mag + (m_z1 * DC_ALPHA);
		pOutData[i] = (z0 - m_z1);
		m_z1 = z0;
	}
	//post filter AM audio to limit high frequency noise
	m_Fir.ProcessFilter(InLength, pOutData, pOutData);
	return InLength;
}

/////////////////////////////////////////////////////////////////////////////////
//	Process envelope AM demod STEREO version
/////////////////////////////////////////////////////////////////////////////////
int CAmDemod::ProcessData(int InLength, TYPECPX* pInData, TYPECPX* pOutData)
{
	for(int i=0; i<InLength; i++)
	{
		//calculate instantaneous power magnitude of pInData which is I*I + Q*Q
		TYPECPX in = pInData[i];
		TYPEREAL mag = sqrt(in.re*in.re+in.im*in.im);
		//High pass filter(DC removal) with IIR filter
		// H(z) = (1 - z^-1)/(1 - ALPHA*z^-1)
		TYPEREAL z0 = mag + (m_z1 * DC_ALPHA);
		pOutData[i].re = (z0 - m_z1);
		pOutData[i].im = (z0 - m_z1);	//make both output channels the same
		m_z1 = z0;
	}
	//post filter AM audio to limit high frequency noise
	m_Fir.ProcessFilter(InLength, pOutData, pOutData);
	return InLength;
}


