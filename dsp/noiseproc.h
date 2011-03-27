//////////////////////////////////////////////////////////////////////
// noiseproc.h: interface for the CNoiseProc class.
//
//  This class implements various noise reduction and blanker functions.
//
// History:
//	2011-01-06  Initial creation MSW
//	2011-03-27  Initial release
//////////////////////////////////////////////////////////////////////
#ifndef NOISEPROC_H
#define NOISEPROC_H

#include "dsp/datatypes.h"
#include <QMutex>

typedef struct _snproc
{
	bool NBOn;
	int NBThreshold;
	int NBWidth;
}tNoiseProcdInfo;

class CNoiseProc
{
public:
	CNoiseProc();
	virtual ~CNoiseProc();

	void SetupBlanker( bool On, TYPEREAL Threshold, TYPEREAL Width, TYPEREAL SampleRate);
	void ProcessBlanker(int InLength, TYPECPX* pInData, TYPECPX* pOutData);

private:
	bool m_On;
	TYPEREAL m_Threshold;
	TYPEREAL m_Width;
	TYPEREAL m_SampleRate;

	TYPECPX* m_DelayBuf;
	TYPEREAL* m_MagBuf;
	TYPECPX* m_TestBenchDataBuf;
	int m_Dptr;
	int m_Mptr;
	int m_BlankCounter;
	int m_DelaySamples;
	int m_MagSamples;
	int m_WidthSamples;
	TYPEREAL m_Ratio;
	TYPEREAL m_MagAveSum;
	TYPEREAL m_MagAve;

	QMutex m_Mutex;		//for keeping threads from stomping on each other

};
#endif //  NOISEPROC_H
