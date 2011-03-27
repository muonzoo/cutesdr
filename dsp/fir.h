//////////////////////////////////////////////////////////////////////
// fir.h: interface for the CFir class.
//
//  This class implements a FIR  filter using a double flat coefficient
//array to eliminate testing for buffer wrap around.
//
// History:
//	2011-01-29  Initial creation MSW
//	2011-03-27  Initial release
//////////////////////////////////////////////////////////////////////
#ifndef FIR_H
#define FIR_H

#include "dsp/datatypes.h"

#define MAX_NUMCOEF 75
#include <QMutex>

class CFir
{
public:
    CFir();

	void InitConstFir( int NumTaps, const double* pCoef);
	int InitLPFilter(TYPEREAL Scale, TYPEREAL Astop, TYPEREAL Fpass, TYPEREAL Fstop, TYPEREAL Fsamprate);
	int InitHPFilter(TYPEREAL Scale, TYPEREAL Astop, TYPEREAL Fpass, TYPEREAL Fstop, TYPEREAL Fsamprate);
	void GenerateHBFilter( TYPEREAL FreqOffset);
	void ProcessFilter(int InLength, TYPEREAL* InBuf, TYPEREAL* OutBuf);
	void ProcessFilter(int InLength, TYPECPX* InBuf, TYPECPX* OutBuf);

private:
	TYPEREAL Izero(TYPEREAL x);
	TYPEREAL m_SampleRate;
	int m_NumTaps;
	int m_State;
	TYPEREAL m_Coef[MAX_NUMCOEF*2];
	TYPEREAL m_ICoef[MAX_NUMCOEF*2];
	TYPEREAL m_QCoef[MAX_NUMCOEF*2];
	TYPEREAL m_rZBuf[MAX_NUMCOEF];
	TYPECPX m_cZBuf[MAX_NUMCOEF];
	QMutex m_Mutex;		//for keeping threads from stomping on each other

};

#endif // FIR_H
