// ssbdemod.cpp: implementation of the CSsbDemod class.
//
//  This class takes I/Q baseband data and performs
// SSB demodulation which doesn't really do anything since ssb is just frequency shifted
// data and the sidebands are rejected by the complex filter in front of this stage.
// so this just copies data from input buffer to outpu buffer.
// Probably should just remove but is here for consistency
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
#include "ssbdemod.h"

CSsbDemod::CSsbDemod()
{
}

int CSsbDemod::ProcessData(int InLength, TYPECPX* pInData, TYPEREAL* pOutData)
{
	for(int i=0; i<InLength ; i++)
		pOutData[i] = pInData[i].re;		//just use real part of signal
	return InLength;
}

int CSsbDemod::ProcessData(int InLength, TYPECPX* pInData, TYPECPX* pOutData)
{
	for(int i=0; i<InLength ; i++)
		pOutData[i] = pInData[i];
	return InLength;
}
