//////////////////////////////////////////////////////////////////////
// netiobase.h: interface for the CNetIOBase,
// CAscpMsg, CUdpThread, CTcpThread, and CIQDataThread classes.
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
/////////////////////////////////////////////////////////////////////
#ifndef NETIOBASE_H
#define NETIOBASE_H

#include <QThread>
#include <QIODevice>
#include <QMutex>
#include <QWaitCondition>
#include <QHostAddress>
#include <QtNetwork>
#include "interface/ascpmsg.h"

#include <QFile>
#include <QDir>


typedef union
{
	struct bs
	{
		unsigned char b0;
		unsigned char b1;
		unsigned char b2;
		unsigned char b3;
	}bytes;
	int all;
}tBtoL4;


//////////////////////////////////////////////////////////////////////
// Low level thread for reading received data bytes from
// UDP Rx Thread class
//////////////////////////////////////////////////////////////////////
class CUdpThread : public QThread
{
	Q_OBJECT
public:

	CUdpThread(QObject* pParent = 0);
protected:
	void run();

private:
	void OnreadyRead();

	quint16 m_LastSeqNum;
	QUdpSocket* m_pUdpSocket;
	QObject* m_pParent;
};

//////////////////////////////////////////////////////////////////////
// Low level TCP Client thread for connecting, and reading received
// data bytes.
//////////////////////////////////////////////////////////////////////
class CTcpThread : public QThread
{
	Q_OBJECT
public:
	CTcpThread(QObject* pParent = 0);
	QTcpSocket* m_pTcpSocket;
	enum eTcpStates {
		TCP_NOT_CONNECTED,
		TCP_CONNECTED,
		TCP_DISCONNECT
	};
	enum eMsgStates {//ASCP msg assembly states
		MSGSTATE_HDR1,
		MSGSTATE_HDR2,
		MSGSTATE_DATA
	};
	eTcpStates m_TcpState;

signals:

protected:
	void run();

private:
	void ManageTcpClientConnection();
	void AssembleAscpMsg(char* pBuf, int length);
	void StartUdp();
	void StopUdp();

	int m_RxMsgLength;
	int m_RxMsgIndex;
	eMsgStates m_MsgState;
	CAscpMsg m_RxAscpMsg;
	CUdpThread* m_pUdpThread;
	QObject* m_pParent;
};



//////////////////////////////////////////////////////////////////////
// Low level thread for reading received data bytes from
// UDP data FIFO and calling IQ processing routine
// This thread performs all the I/Q data DSP processing.
//////////////////////////////////////////////////////////////////////
class CIQDataThread : public QThread
{
	Q_OBJECT
public:
	CIQDataThread(QObject* pParent = 0);
	~CIQDataThread();
protected:
	void run();
private:
	QObject* m_pParent;
	void FileTest();
	QFile m_File;
};

//////////////////////////////////////////////////////////////////////
//Base Class to derive from a radio specific interface.  Provides only
// low level message assembly and I/Q data message buffering.
// User would derive from this class to process all the radio messages
//////////////////////////////////////////////////////////////////////
class CNetIOBase : public QObject
{
	Q_OBJECT
public:
	CNetIOBase();
	virtual ~CNetIOBase();

	enum eStatus {
		NOT_CONNECTED,
		CONNECTED,
		RUNNING,
		ERROR
	};

	//stub virtual function gets implemented by specific device sub class implementation
	virtual void ParseAscpMsg( CAscpMsg*){}	//implement to decode all the command/status messages
	virtual void SendIOStatus(int ){}		//implement to process IO status/error changes
	virtual void ProcessIQData( double* , int ){}//implement to process the IQ data messages from the radio

	void StartIO();	//starts IO threads
	void StopIO();	//stops IO threads

	void SendAscpMsg(CAscpMsg* pMsg);	//sends msg to radio
	void SetupNetwork(QHostAddress ip4Addr, quint16 port);	//set network parameters

public:
	bool m_TcpThreadQuit;
	bool m_UdpThreadQuit;
	bool m_IQDataThreadQuit;
	bool m_SampleSize24;
	int m_RxQueueHead;
	int m_RxQueueTail;
	int m_MissedPackets;
	quint16 m_Port;
	double **m_pUdpRxQueue;
	QHostAddress m_IPAdr;
	QWaitCondition m_QWaitFifoData;
	QMutex m_TcpMutex;
	QMutex m_UdpRxMutex;
	CTcpThread* m_pTcpThread;
	CIQDataThread* m_pIQDataThread;

protected:

private:
};

#endif // NETIOBASE_H
