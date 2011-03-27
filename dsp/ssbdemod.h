//////////////////////////////////////////////////////////////////////
// ssbdemod.h: interface for the CSsbDemod class.
//
// History:
//	2010-09-22  Initial creation MSW
//	2011-03-27  Initial release
/////////////////////////////////////////////////////////////////////
#ifndef SSBDEMOD_H
#define SSBDEMOD_H
#include "dsp/datatypes.h"

class CSsbDemod
{
public:
	CSsbDemod();
	int ProcessData(int InLength, TYPECPX* pInData, TYPEREAL* pOutData);
	int ProcessData(int InLength, TYPECPX* pInData, TYPECPX* pOutData);
private:
};

#endif // SSBDEMOD_H
