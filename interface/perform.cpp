///////////////////////////////////////////////////////
// Perform.cpp : implementation file
//
//  NOTE:: these functions depend on the 64 bit performance timer
// found in Pentium processors. This timer may not be present
//  in some of the non-Intel versions.
// Also it may not work with multi core CPU's across threads.
//
// History:
//	2010-11-10  Initial creation MSW
//	2011-03-27  Initial release
////////////////////////////////////////////////////////////////////////

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
#include "interface/perform.h"
#include <QtGlobal>
#include <QDebug>

#define CPUCLOCKRATEGHZ 3		//manually set to CPU core clock speed

/////////////////////////////////////////////////////////////////////////////
static quint64 StartTime;
static quint64 StopTime;
static quint64 DeltaTime;
static quint64 CountFreq;
static quint64 DeltaTimeMax;
static quint64 DeltaTimeMin;
static quint64 DeltaTimeAve;
static quint64 DeltaSamples;
static quint64 Length;

////////////////  Time measuring routine using Pentium timer
static quint64 QueryPerformanceCounter()
{
quint32 eax, edx;
quint64 val;
	__asm__ __volatile__("cpuid": : : "ax", "bx", "cx", "dx");
	__asm__ __volatile__("rdtsc":"=a"(eax), "=d"(edx));
	val = edx;
	val = val << 32;
	val += eax;
	return val;
}

// call to initialize the prformance timer
void InitPerformance()
{
	Length = 0;
	DeltaTimeMax = 0;
	DeltaTimeAve = 0;
	DeltaSamples = 0;
	DeltaTimeMin = 0x7FFFFFF;
	CountFreq = CPUCLOCKRATEGHZ;
}

// Starts the performance timer
void StartPerformance()
{
	StartTime = QueryPerformanceCounter();
}

// Stop performance timer and calculate timing values
void StopPerformance(int n)
{
	StopTime = QueryPerformanceCounter();
	DeltaTime = (StopTime-StartTime);
	DeltaTimeAve += DeltaTime;
	DeltaSamples++;
	if( DeltaTime>DeltaTimeMax )
	{
		DeltaTimeMax = DeltaTime;
	}
	if( DeltaTime<DeltaTimeMin )
	{
		DeltaTimeMin = DeltaTime;
	}
	Length += n;
}

// Call this to measure time between succesive calls to SamplePerformance()
void SamplePerformance()
{
	if(	DeltaTimeMax == 0 )
	{
		StartTime = QueryPerformanceCounter();
		DeltaTimeMax = 1;
	}
	else
	{
		StopTime = QueryPerformanceCounter();
		DeltaTime = StopTime-StartTime;
		DeltaTimeAve += DeltaTime;
		if( DeltaTime>DeltaTimeMax )
		{
			DeltaTimeMax = DeltaTime;
		}
		if( DeltaTime<DeltaTimeMin )
		{
			DeltaTimeMin = DeltaTime;
		}
		StartTime = QueryPerformanceCounter();
		DeltaSamples++;
		Length++;
	}
}

// output various timing statistics to Message Box
// DeltaTimeMax == maximum time between start()-stop() or sample()-Sample()
// DeltaTimeMin == minimum time between start()-stop() or sample()-Sample()
// DeltaTimeAve == average time between start()-stop() or sample()-Sample()
// DeltaSamples == number of time samples captured
void ReadPerformance()
{
	if(DeltaSamples != 0 )
	{
		DeltaTime = (DeltaTime)/CountFreq;
		DeltaTimeMin = (DeltaTimeMin)/CountFreq;
		DeltaTimeMax = (DeltaTimeMax)/CountFreq;
		DeltaTimeAve = (DeltaTimeAve)/CountFreq;

		float fAve= (float)DeltaTimeAve/( (float)Length);
		qDebug()<<"Length "<<Length/DeltaSamples;
		if(fAve>1e6)
		{
			qDebug()<<"Delta Time Max mSec = "<<DeltaTimeMax/1000000  <<"Delta Time Min mSec = "<<DeltaTimeMin/1000000;
			qDebug()<<"Ave mSec= "<<fAve/1000000.0<<" #Samples = "<< DeltaSamples;
		}
		else if(fAve>1e3)
		{
			qDebug()<<"Delta Time Max uSec = "<<DeltaTimeMax/1000  <<"Delta Time Min uSec = "<<DeltaTimeMin/1000;
			qDebug()<<"Ave uSec= "<<fAve/1000.0<<" #Samples = "<< DeltaSamples;
		}
		else
		{
			qDebug()<<"Delta Time Max nSec = "<<DeltaTimeMax  <<"Delta Time Min nSec = "<<DeltaTimeMin;
			qDebug()<<"Ave nSec= "<<fAve<<" #Samples = "<< DeltaSamples;
		}
	}
}

int GetDeltaPerformance()
{
quint64 delta = ((StopTime-StartTime))/CountFreq;
	return (int)delta;
}



