//////////////////////////////////////////////////////////////////////
// iir.h: interface for the CIir class.
//
//  This class implements an IIR  filter
//
// History:
//	2011-02-05  Initial creation MSW
//	2011-03-27  Initial release
//////////////////////////////////////////////////////////////////////
#ifndef IIR_H
#define IIR_H

#include "dsp/datatypes.h"


class CIir
{
public:
	CIir();

	void InitLP( TYPEREAL F0Freq, TYPEREAL FilterQ, TYPEREAL SampleRate);	//create Low Pass
	void InitHP( TYPEREAL F0Freq, TYPEREAL FilterQ, TYPEREAL SampleRate);	//create High Pass
	void InitBP( TYPEREAL F0Freq, TYPEREAL FilterQ, TYPEREAL SampleRate);	//create Band Pass
	void InitBR( TYPEREAL F0Freq, TYPEREAL FilterQ, TYPEREAL SampleRate);	//create Band Reject
	void ProcessFilter(int InLength, TYPEREAL* InBuf, TYPEREAL* OutBuf);
	void ProcessFilter(int InLength, TYPECPX* InBuf, TYPECPX* OutBuf);

private:
	TYPEREAL m_SampleRate;
	TYPEREAL m_A1;		//direct form 2 coefficients
	TYPEREAL m_A2;
	TYPEREAL m_B0;
	TYPEREAL m_B1;
	TYPEREAL m_B2;

	TYPEREAL m_w1a;		//biquad delay storage
	TYPEREAL m_w2a;
	TYPEREAL m_w1b;		//biquad delay storage
	TYPEREAL m_w2b;
};

#endif // IIR_H
