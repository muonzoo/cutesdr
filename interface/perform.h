///////////////////////////////////////////////////////
// Perform.h : Global interface for the performance functions
// History:
//	2010-11-10  Initial creation MSW
//	2011-03-27  Initial release
//////////////////////////////////////////////////////////////////////
//
#if !defined(_INCLUDE_PERFORMXXX_H_)
#define _INCLUDE_PERFORMXXX_H_
#include "math.h"
/////////////////////////////////////////////////////////////////////////////
extern void InitPerformance();
extern void StartPerformance();
extern void StopPerformance(int n);
extern void ReadPerformance();
extern void SamplePerformance();
extern int GetDeltaPerformance();
#endif //#if !defined(_INCLUDE_PERFORMXXX_H_)
