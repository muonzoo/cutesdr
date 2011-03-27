//////////////////////////////////////////////////////////////////////
// amdemod.h: interface for the CAmDemod class.
//
// History:
//	2010-09-22  Initial creation MSW
//	2011-03-27  Initial release
/////////////////////////////////////////////////////////////////////
#ifndef AMDEMOD_H
#define AMDEMOD_H
#include "dsp/datatypes.h"
#include "dsp/fir.h"

class CAmDemod
{
public:
	CAmDemod(TYPEREAL samplerate);
	void SetBandwidth(TYPEREAL Bandwidth);
	//overloaded functions for mono and stereo
	int ProcessData(int InLength, TYPECPX* pInData, TYPEREAL* pOutData);
	int ProcessData(int InLength, TYPECPX* pInData, TYPECPX* pOutData);
private:
	TYPEREAL m_SampleRate;
	TYPEREAL m_z1;
	CFir m_Fir;
};

#endif // AMDEMOD_H
