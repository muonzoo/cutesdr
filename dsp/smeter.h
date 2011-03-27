//////////////////////////////////////////////////////////////////////
// smeter.h: interface for the CSMeter class.
//
// History:
//	2010-09-22  Initial creation MSW
//	2011-03-27  Initial release
/////////////////////////////////////////////////////////////////////
#ifndef SMETER_H
#define SMETER_H
#include "dsp/datatypes.h"

class CSMeter
{
public:
    CSMeter();
	void ProcessData(int length, TYPECPX* pInData, TYPEREAL SampleRate);
	TYPEREAL GetPeak();
	TYPEREAL GetAve();

private:
	TYPEREAL m_AverageMag;
	TYPEREAL m_PeakMag;
	TYPEREAL m_SampleRate;
	TYPEREAL m_AttackAve;
	TYPEREAL m_DecayAve;
	TYPEREAL m_AttackAlpha;
	TYPEREAL m_DecayAlpha;

};

#endif // SMETER_H
