//////////////////////////////////////////////////////////////////////
// fmdemod.h: interface for the CFmDemod class.
//
// History:
//	2011-01-17  Initial creation MSW
//	2011-03-27  Initial release
/////////////////////////////////////////////////////////////////////
#ifndef FMDEMOD_H
#define FMDEMOD_H
#include "dsp/datatypes.h"
#include "dsp/fir.h"
#include "dsp/iir.h"

#define MAX_SQBUF_SIZE 16384

class CFmDemod
{
public:
	CFmDemod(TYPEREAL samplerate);
	//overloaded functions for mono and stereo
	int ProcessData(int InLength, TYPEREAL FmBW, TYPECPX* pInData, TYPECPX* pOutData);
	int ProcessData(int InLength, TYPEREAL FmBW, TYPECPX* pInData, TYPEREAL* pOutData);

	void SetSquelch(int Value);		//call with range of 0 to 99 to set squelch threshold

private:
	void PerformNoiseSquelch(int InLength, TYPECPX* pOutData);
	void PerformNoiseSquelch(int InLength, TYPEREAL* pOutData);
	void InitNoiseSquelch();

	bool m_SquelchState;
	TYPEREAL m_SampleRate;
	TYPEREAL m_SquelchHPFreq;
	TYPEREAL m_OutGain;
	TYPEREAL m_FreqErrorDC;
	TYPEREAL m_DcAlpha;
	TYPEREAL m_NcoPhase;
	TYPEREAL m_NcoFreq;
	TYPEREAL m_NcoAcc;
	TYPEREAL m_NcoLLimit;
	TYPEREAL m_NcoHLimit;
	TYPEREAL m_PllAlpha;
	TYPEREAL m_PllBeta;

	TYPEREAL m_SquelchThreshold;
	TYPEREAL m_SquelchAve;
	TYPEREAL m_SquelchAlpha;

	TYPEREAL m_OutBuf[MAX_SQBUF_SIZE];

	CFir m_HpFir;
	CIir m_LpIir;

};

#endif // FMDEMOD_H
