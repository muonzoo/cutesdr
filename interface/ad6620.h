//////////////////////////////////////////////////////////////////////
// ad6620.h: interface for the Cad6620 class.
// Needed only for sdr-14 and sdr-iq radios
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
/////////////////////////////////////////////////////////////////////
#ifndef AD6620_H
#define AD6620_H

#include "interface/netiobase.h"


#define MAX_COEF 256
#define MAX_MSGS 512

class Cad6620
{
public:
	Cad6620();
	virtual ~Cad6620();
	enum eSAMPLERATE
	{
		BWKHZ_5,
		BWKHZ_10,
		BWKHZ_25,
		BWKHZ_50,
		BWKHZ_100,
		BWKHZ_150,
		BWKHZ_190,
		BWKHZ_250,
		BWKHZ_500,
		BWKHZ_1000,
		BWKHZ_1500,
		BWKHZ_2000,
		BWKHZ_4000
	};

	void CreateLoad6620Msgs(int filter);		//called to fill up msg buffer with all msgs needed to load 'filter'
	bool GetNext6620Msg(CAscpMsg &pAscpMsg);	//call to get formatted msg to send to radio


private:
	void PutInMsgBuffer(int adr, quint32 data);
	struct mq
	{
		quint16 adr;
		quint32 data;
		quint8 datah;
	}m_MsgBuffer[MAX_MSGS];
	int m_BufIndx;
	int m_BufLength;
	bool m_NCOphzDither;
	bool m_NCOampDither;
	int	m_UseableBW;
	int	m_CIC2Rate;
	int	m_CIC2Scale;
	int	m_CIC5Rate;
	int	m_CIC5Scale;
	int	m_RCFRate;
	int	m_RCFScale;
	int m_RCFTaps;
	int m_FIRcoef[MAX_COEF];
	int m_TotalDecimationRate;
};

#endif // AD6620_H
