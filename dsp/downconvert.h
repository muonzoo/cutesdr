// downconvert.h: interface for the CDownConvert class.
//
//  This class takes I/Q baseband data and performs tuning
//(Frequency shifting of the baseband signal) as well as
// decimation in powers of 2 after the shifting.
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
//////////////////////////////////////////////////////////////////////
#ifndef DOWNCONVERT_H
#define DOWNCONVERT_H

#include "dsp/datatypes.h"
#include <QMutex>


#define MAX_DECSTAGES 10	//one more than max to make sure is a null at end of list

//////////////////////////////////////////////////////////////////////////////////
// Main Downconverter Class
//////////////////////////////////////////////////////////////////////////////////
class CDownConvert  
{
public:
	CDownConvert();
	virtual ~CDownConvert();
	void SetFrequency(TYPEREAL NcoFreq);
	void SetCwOffset(TYPEREAL offset){m_CW_Offset= offset;}
	int ProcessData(int InLength, TYPECPX* pInData, TYPECPX* pOutData);
	TYPEREAL SetDataRate(TYPEREAL InRate, TYPEREAL MaxBW);

private:
	////////////
	//pure abstract base class for all the different types of decimate by 2 stages
	//DecBy2 function is defined in derived classes
	////////////
	class CDec2
	{
	public:
		CDec2(){}
		virtual ~CDec2(){}
		virtual int DecBy2(int InLength, TYPECPX* pInData, TYPECPX* pOutData) = 0;
	};

	////////////
	//private class for the Half Band decimate by 2 stages
	////////////
	class CHalfBandDecimateBy2 : public CDec2
	{
	public:
		CHalfBandDecimateBy2(int len,const TYPEREAL* pCoef);
		~CHalfBandDecimateBy2(){if(m_pHBFirBuf) delete m_pHBFirBuf;}
		int DecBy2(int InLength, TYPECPX* pInData, TYPECPX* pOutData);
		TYPECPX* m_pHBFirBuf;
		int m_FirLength;
		const TYPEREAL* m_pCoef;
	};


	////////////
	//private class for the fixed 11 tap Half Band decimate by 2 stages
	////////////
	class CHalfBand11TapDecimateBy2 : public CDec2
	{
	public:
		CHalfBand11TapDecimateBy2();
		~CHalfBand11TapDecimateBy2(){}
		int DecBy2(int InLength, TYPECPX* pInData, TYPECPX* pOutData);
		TYPEREAL H0;	//unwrapped coeeficients
		TYPEREAL H2;
		TYPEREAL H4;
		TYPEREAL H5;
		TYPEREAL H6;
		TYPEREAL H8;
		TYPEREAL H10;
		TYPECPX d0;		//unwrapped delay buffer
		TYPECPX d1;
		TYPECPX d2;
		TYPECPX d3;
		TYPECPX d4;
		TYPECPX d5;
		TYPECPX d6;
		TYPECPX d7;
		TYPECPX d8;
		TYPECPX d9;
	};

	////////////
	//private class for the N=3 CIC decimate by 2 stages
	////////////
	class CCicN3DecimateBy2 : public CDec2
	{
	public:
		CCicN3DecimateBy2();
		~CCicN3DecimateBy2(){}
		int DecBy2(int InLength, TYPECPX* pInData, TYPECPX* pOutData);
		TYPECPX m_Xodd;
		TYPECPX m_Xeven;
	};

private:
	//private helper functions
	void DeleteFilters();

	TYPEREAL m_OutputRate;
	TYPEREAL m_NcoFreq;
	TYPEREAL m_CW_Offset;
	TYPEREAL m_NcoInc;
	TYPEREAL m_NcoTime;
	TYPEREAL m_InRate;
	TYPEREAL m_MaxBW;
	TYPECPX m_Osc1;
	TYPEREAL m_OscCos;
	TYPEREAL m_OscSin;
	QMutex m_Mutex;		//for keeping threads from stomping on each other
	//array of pointers for performing decimate by 2 stages
	CDec2* m_pDecimatorPtrs[MAX_DECSTAGES];

};

#endif // DOWNCONVERT_H
