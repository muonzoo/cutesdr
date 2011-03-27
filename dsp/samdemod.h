//////////////////////////////////////////////////////////////////////
// samdemod.h: interface for the CSamDemod class.
//
// History:
//	2010-09-22  Initial creation MSW
//	2011-03-27  Initial release
/////////////////////////////////////////////////////////////////////
#ifndef SAMDEMOD_H
#define SAMDEMOD_H
#include "dsp/datatypes.h"
#include "dsp/fir.h"

class CSamDemod
{
public:
	CSamDemod(TYPEREAL samplerate);
	//overloaded functions for mono and stereo
	int ProcessData(int InLength, TYPECPX* pInData, TYPEREAL* pOutData);
	int ProcessData(int InLength, TYPECPX* pInData, TYPECPX* pOutData);
private:
	TYPEREAL m_SampleRate;
	TYPEREAL m_z1;
	TYPEREAL m_y1;
	TYPEREAL m_NcoPhase;
	TYPEREAL m_NcoFreq;
	TYPEREAL m_NcoAcc;
	TYPEREAL m_NcoLLimit;
	TYPEREAL m_NcoHLimit;
	TYPEREAL m_PllAlpha;
	TYPEREAL m_PllBeta;
	CFir m_Fir;
};

#endif // SAMDEMOD_H
