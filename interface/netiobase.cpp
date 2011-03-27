//////////////////////////////////////////////////////////////////////
// netiobase.cpp: implementation of the CNetIOBase class.
//
// Network I/O module base class( Only used to derive one's own class)
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
////////////////////////////////////////////////////////////////////////////////////////////

//==========================================================================================
// + + +   This Software is released under the "Simplified BSD License"  + + +
//Copyright 2010 Moe Wheatley. All rights reserved.
//
//Redistribution and use in source and binary forms, with or without modification, are
//permitted provided that the following conditions are met:
//
//   1. Redistributions of source code must retain the above copyright notice, this list of
//	  conditions and the following disclaimer.
//
//   2. Redistributions in binary form must reproduce the above copyright notice, this list
//	  of conditions and the following disclaimer in the documentation and/or other materials
//	  provided with the distribution.
//
//THIS SOFTWARE IS PROVIDED BY Moe Wheatley ``AS IS'' AND ANY EXPRESS OR IMPLIED
//WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
//FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL Moe Wheatley OR
//CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
//ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
//ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//The views and conclusions contained in the software and documentation are those of the
//authors and should not be interpreted as representing official policies, either expressed
//or implied, of Moe Wheatley.
//==========================================================================================

/*---------------------------------------------------------------------------*/
/*------------------------> I N C L U D E S <--------------------------------*/
/*---------------------------------------------------------------------------*/
#include "interface/netiobase.h"
#include <QtDebug>


#ifdef Q_WS_WIN
#include "winsock2.h"
#else
#include <sys/socket.h>
#include <sys/socketvar.h>
#endif

/*---------------------------------------------------------------------------*/
/*--------------------> L O C A L   D E F I N E S <--------------------------*/
/*---------------------------------------------------------------------------*/
#define PKT_LENGTH_24 1444
#define PKT_LENGTH_16 1028

#define RXQUEUE_SIZE 256		//queue size(keep power of 2)



/*---------------------------------------------------------------------------*/
/*----------------> L O C A L   S T R U C T U R E S  <-----------------------*/
/*---------------------------------------------------------------------------*/
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
}tBtoL;

typedef union
{
	struct bs
	{
		unsigned char b0;
		unsigned char b1;
	}bytes;
	signed short sall;
	unsigned short all;
}tBtoS;

//&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+
// NetIOBase class implementation.
//&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+
CNetIOBase::CNetIOBase()
{
	m_pUdpRxQueue = NULL;
	m_pTcpThread = NULL;
	m_pIQDataThread = NULL;
	m_RxQueueHead = 0;
	m_RxQueueTail = 0;
	m_TcpThreadQuit = FALSE;
	m_IQDataThreadQuit = FALSE;
	m_UdpThreadQuit = FALSE;
	m_SampleSize24 = FALSE;
}

CNetIOBase::~CNetIOBase()
{
	StopIO();
}

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*                   -------------------------                               */
/*                  | S e t u p N e t w o r k |                              */
/*                   -------------------------                               */
/*  Set client network parameters (call before starting IO)                  */
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
void CNetIOBase::SetupNetwork(QHostAddress ip4Addr, quint16 port)
{
	m_IPAdr = ip4Addr;
	m_Port = port;
qDebug()<<m_IPAdr;
qDebug()<<"Port = "<< m_Port;
	if(NULL != m_pTcpThread)
		m_pTcpThread->m_TcpState = CTcpThread::TCP_DISCONNECT;	//force reconnect with new parameters
}

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*                         ---------------                                   */
/*                        | S t a r t I O |                                  */
/*                         ---------------                                   */
/*  Starts up IO threads                                                     */
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
void CNetIOBase::StartIO()
{
qDebug()<<"Start I/O";

	if(!m_pUdpRxQueue)
	{	//create 2D array for FIFO
		m_pUdpRxQueue = new double* [RXQUEUE_SIZE];		//create array of pointers to each row
		for(int i=0; i<RXQUEUE_SIZE; i++)
		{	//now allocate memory for each row
			m_pUdpRxQueue[i] = new double[PKT_LENGTH_24];	//enough for max packet data length
		}
	}
	for(int i=0; i<RXQUEUE_SIZE; i++)
		for(int j=0; j<PKT_LENGTH_24; j++)
			m_pUdpRxQueue[i][j] = 0.0;

	m_RxQueueHead = 0;
	m_RxQueueTail = 0;
	m_TcpThreadQuit = FALSE;
	if(NULL != m_pTcpThread)
	{
		delete m_pTcpThread;
		m_pTcpThread = NULL;
	}
	m_pTcpThread = new CTcpThread(this);
	m_pTcpThread->start();


	m_IQDataThreadQuit = FALSE;
	if(NULL != m_pIQDataThread)
	{
		delete m_pIQDataThread;
		m_pIQDataThread = NULL;
	}
	m_pIQDataThread = new CIQDataThread(this);
	m_pIQDataThread->start(QThread::HighestPriority);	//this thread does all the DSP processing
	m_MissedPackets = 0;
}


/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*                         -------------                                     */
/*                        | S t o p I O |                                    */
/*                         -------------                                     */
/*  Stops IO threads                                                         */
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
void CNetIOBase::StopIO()
{
	m_TcpThreadQuit = TRUE;
	if(NULL != m_pTcpThread)
	{
		m_pTcpThread->wait(5000);	//wait for tcp thread to exit then destroy
		delete m_pTcpThread;
		m_pTcpThread = NULL;
qDebug()<<"tcpthread stopped";
	}

	m_IQDataThreadQuit = TRUE;
	if(NULL != m_pIQDataThread)
	{
		m_RxQueueHead = 0;
		m_RxQueueTail = 0;
		m_pIQDataThread->wait(1000);	//wait for Fifo thread to exit then destroy
		delete m_pIQDataThread;
		m_pIQDataThread = NULL;
qDebug()<<"fifothread stopped";
	}

	if(NULL != m_pUdpRxQueue)		//delete FIFO memory
	{
		for(int i=0; i<RXQUEUE_SIZE; i++)	//first delete all the row memory
			delete [] m_pUdpRxQueue[i];
		delete [] m_pUdpRxQueue;				//delete column ptr memory
		m_pUdpRxQueue = NULL;
	}
}


/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*                     -----------------------                               */
/*                    | S e n d A s c p M s g |                              */
/*                     -----------------------                               */
/*   Called send an ASCP formated message to the TCP network Device          */
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
void CNetIOBase::SendAscpMsg(CAscpMsg* pMsg)
{
	if(!m_pTcpThread)
		return;
	if( m_pTcpThread->m_TcpState ==  CTcpThread::TCP_CONNECTED)
	{
//qDebug()<<"send";
		qint64 txbytes = pMsg->Buf16[0] & LENGTH_MASK;
		m_TcpMutex.lock();
		qint64 sent = m_pTcpThread->m_pTcpSocket->write((const char*)pMsg->Buf8, txbytes);
		m_TcpMutex.unlock();
		if(txbytes!=sent)
			qDebug()<<"tcp write error";
	}
}


//&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+
// TCP thread class implementation.
//&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+
CTcpThread::CTcpThread(QObject* pParent) : m_pParent(pParent)
{
	m_pTcpSocket = NULL;
	m_pUdpThread = NULL;
	m_TcpState = TCP_NOT_CONNECTED;
	m_RxMsgLength = 0;
	m_RxMsgIndex = 0;
	m_MsgState = MSGSTATE_HDR1;
}

//////////////////////////////////////////////////////////////////////////
//TCP Client operation thread.  manages connection state and Rx msgs
//////////////////////////////////////////////////////////////////////////
void CTcpThread::run()
{
CNetIOBase* pParent = (CNetIOBase*)m_pParent;
	m_pTcpSocket = new QTcpSocket;
	m_TcpState = TCP_NOT_CONNECTED;
	while(!pParent->m_TcpThreadQuit)
	{
		ManageTcpClientConnection();
	}
	delete m_pTcpSocket;
	m_pTcpSocket = NULL;
	StopUdp();
}

void CTcpThread::StartUdp()
{
	CNetIOBase* pParent = (CNetIOBase*)m_pParent;
	pParent->m_UdpThreadQuit = FALSE;
	if(NULL != m_pUdpThread)
	{
		qDebug()<<"udpthread already running";
		delete m_pUdpThread;
		m_pUdpThread = NULL;
	}
	m_pUdpThread = new CUdpThread(m_pParent);
//	m_pUdpThread->start(QThread::TimeCriticalPriority);
	m_pUdpThread->start(QThread::HighestPriority);
}

void CTcpThread::StopUdp()
{
CNetIOBase* pParent = (CNetIOBase*)m_pParent;
	pParent->m_UdpThreadQuit = TRUE;
	if(NULL != m_pUdpThread)
	{
		m_pUdpThread->wait(1000);	//wait for udp thread to exit then destroy
		delete m_pUdpThread;
		m_pUdpThread = NULL;
qDebug()<<"udpthread stopped";
	}
}


/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*          ---------------------------------------------------              */
/*         | M a n a g e T c p C l i e n t C o n n e c t i o n |             */
/*          ---------------------------------------------------              */
/*  Helper function manages TCP client connect logic                         */
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
void  CTcpThread::ManageTcpClientConnection()
{
int Timeout;
char Buf[2048];
qint64 bytesavailable;
CNetIOBase* pParent = (CNetIOBase*)m_pParent;
	switch(m_TcpState)
	{
		case TCP_NOT_CONNECTED:
			qDebug()<<"Try to connect";
			m_pTcpSocket->connectToHost(pParent->m_IPAdr, pParent->m_Port);
			if (m_pTcpSocket->waitForConnected(500))
			{
				qDebug()<<"Connected";
				StartUdp();
				m_TcpState = TCP_CONNECTED;
				m_RxMsgLength = 0;
				m_RxMsgIndex = 0;
				m_MsgState = MSGSTATE_HDR1;
				pParent->SendIOStatus(CNetIOBase::CONNECTED);
			}
			else
			{	//wait 2 seconds before trying again yet break out sooner if thread is stopped
				pParent->SendIOStatus(CNetIOBase::NOT_CONNECTED);
				Timeout = 2*10;
				while( (!pParent->m_TcpThreadQuit) && Timeout--)
					msleep(100);
			}
			break;
		case TCP_CONNECTED:
			if(m_pTcpSocket->waitForReadyRead(100) )
			{
				pParent->m_TcpMutex.lock();
				bytesavailable = m_pTcpSocket->bytesAvailable();
				pParent->m_TcpMutex.unlock();
				if(bytesavailable)
				{
					pParent->m_TcpMutex.lock();
					m_pTcpSocket->read(Buf, bytesavailable);
					pParent->m_TcpMutex.unlock();
					AssembleAscpMsg(Buf, (int)bytesavailable);
				}
			}
			if(m_pTcpSocket->state() != QAbstractSocket::ConnectedState)
			{
				m_pTcpSocket->close();
				qDebug()<<"connect error";
				pParent->SendIOStatus(CNetIOBase::NOT_CONNECTED);
				m_TcpState = TCP_NOT_CONNECTED;
				StopUdp();
			}
			break;
		case TCP_DISCONNECT:
			msleep(100);	//wait for any pending msgs to get sent
			if(m_pTcpSocket->state() == QAbstractSocket::ConnectedState)
			{
				m_pTcpSocket->disconnectFromHost();
			}
			m_pTcpSocket->close();
			qDebug()<<"Not Connected";
			pParent->SendIOStatus(CNetIOBase::NOT_CONNECTED);
			m_TcpState = TCP_NOT_CONNECTED;
			StopUdp();
			break;
		default:
			break;
	}
	if(pParent->m_TcpThreadQuit && (m_pTcpSocket->state() != QAbstractSocket::UnconnectedState) )
	{	//if thread stopped and is connected then disconnect TCP socket
		msleep(100);	//wait for any pending msgs to get sent
		m_pTcpSocket->disconnectFromHost();
		m_pTcpSocket->waitForDisconnected(5000);
		m_pTcpSocket->close();
		qDebug()<<"Not Connected";
		pParent->SendIOStatus(CNetIOBase::NOT_CONNECTED);
	}
}


/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
/*                  -------------------------------                          */
/*                 | A s s e m b l e A s c p M s g |                         */
/*                  -------------------------------                          */
/*  Helper function to assemble TCP data stream into ASCP formatted messages */
/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
void CTcpThread::AssembleAscpMsg(char* pBuf, int length)
{
CNetIOBase* pParent = (CNetIOBase*)m_pParent;
	for( int i=0; i<length;i++)
	{	//process everything in Queue
		quint8 data = pBuf[i];
		switch(m_MsgState)	//state machine to get generic ASCP msg from TCP data stream
		{
			case MSGSTATE_HDR1:		//get first header byte of ASCP msg
				m_RxAscpMsg.Buf8[0] = data;
				m_MsgState = MSGSTATE_HDR2;	//go to get second header byte state
				break;
			case MSGSTATE_HDR2:	//here for second byte of header
				m_RxAscpMsg.Buf8[1] = data;
				m_RxMsgLength = m_RxAscpMsg.Buf16[0] & LENGTH_MASK;
				m_RxMsgIndex = 2;
				if(2 == m_RxMsgLength)	//if msg has no parameters then we are done
				{
					m_MsgState = MSGSTATE_HDR1;	//go back to first state
					pParent->ParseAscpMsg( &m_RxAscpMsg );
				}
				else	//there are data bytes to fetch
				{
					if( 0 == m_RxMsgLength)
						m_RxMsgLength = 8192+2;	//handle special case of 8192 byte data message
					m_MsgState = MSGSTATE_DATA;	//go to data byte reading state
				}
				break;
			case MSGSTATE_DATA:	//try to read the rest of the message
				m_RxAscpMsg.Buf8[m_RxMsgIndex++] = data;
				if( m_RxMsgIndex >= m_RxMsgLength )
				{
					m_MsgState = MSGSTATE_HDR1;	//go back to first stage
					m_RxMsgIndex = 0;
					pParent->ParseAscpMsg( &m_RxAscpMsg );
				}
				break;
		} //end switch statement
	}
}

//&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+
// UDP thread class implementation.
//&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+
CUdpThread::CUdpThread(QObject* pParent) : m_pParent(pParent)
{
	m_pUdpSocket = NULL;
	m_LastSeqNum = 0;
}

//////////////////////////////////////////////////////////////////////////
// UDP thread waits for UDP Rx data
//////////////////////////////////////////////////////////////////////////
void CUdpThread::run()
{
CNetIOBase* pParent = (CNetIOBase*)m_pParent;
	m_pUdpSocket = new QUdpSocket;
	if(!m_pUdpSocket->bind(pParent->m_Port,QUdpSocket::ShareAddress) )
	{
		qDebug()<<"UDP Bind Failed";
	}
	int v = 2000000;
	::setsockopt(m_pUdpSocket->socketDescriptor(), SOL_SOCKET, SO_RCVBUF, (char *)&v, sizeof(v));
	while(!pParent->m_UdpThreadQuit)
	{
		if(m_pUdpSocket->waitForReadyRead(100) )
		{
			OnreadyRead();
		}
	}
	m_pUdpSocket->close();
	delete m_pUdpSocket;
	m_pUdpSocket = NULL;
}


//////////////////////////////////////////////////////////////////////////
// Called when UDP Rx data is available then converts to float and puts in FIFO
//////////////////////////////////////////////////////////////////////////
void CUdpThread::OnreadyRead()
{
char Buf[2048];
tBtoL data;
qint64 size;
int i,j;
tBtoS seq;
CNetIOBase* pParent = (CNetIOBase*)m_pParent;
//Q_ASSERT(QThread::currentThread() == m_pUdpSocket->thread());	//just see if really is a direct call by the socket thread
	data.all = 0;
	seq.all = 0;
	pParent->m_UdpRxMutex.lock();
	while( m_pUdpSocket->hasPendingDatagrams() )
	{
		size = m_pUdpSocket->pendingDatagramSize();
		if(PKT_LENGTH_24 == size)
		{	//24 bit I/Q data
			pParent->m_SampleSize24 = TRUE;
			m_pUdpSocket->readDatagram( Buf, 2048, 0, 0 );
			seq.bytes.b0 = Buf[2];
			seq.bytes.b1 = Buf[3];
			if(0==seq.all)	//is first packet after started
				m_LastSeqNum = 0;
			if(seq.all != m_LastSeqNum)
			{
//qDebug()<<seq.all <<m_LastSeqNum;
				pParent->m_MissedPackets += ((qint16)seq.all - (qint16)m_LastSeqNum);
				m_LastSeqNum = seq.all;
			}
			m_LastSeqNum++;
			if(0==m_LastSeqNum)
				m_LastSeqNum = 1;
			for( i=4,j=0; i<size; i+=3,j++)
			{
				data.bytes.b1 = Buf[i];		//combine 3 bytes into 32 bit signed int
				data.bytes.b2 = Buf[i+1];
				data.bytes.b3 = Buf[i+2];
				pParent->m_pUdpRxQueue[pParent->m_RxQueueHead][j] = (double)data.all/65536.0;	//scale to be +-32768 range same as 16 bit data
			}
		}
		else if(PKT_LENGTH_16 == size)
		{	//16 bit I/Q data
			pParent->m_SampleSize24 = FALSE;
			m_pUdpSocket->readDatagram( Buf, 2048, 0, 0 );
			seq.bytes.b0 = Buf[2];
			seq.bytes.b1 = Buf[3];
			if(0==seq.all)	//is first packet after started
				m_LastSeqNum = 0;
			if(seq.all != m_LastSeqNum)
			{
//qDebug()<<seq.all <<m_LastSeqNum;
				pParent->m_MissedPackets += ((qint16)seq.all - (qint16)m_LastSeqNum);
				m_LastSeqNum = seq.all;
			}
			m_LastSeqNum++;
			if(0==m_LastSeqNum)
				m_LastSeqNum = 1;
			for( i=4,j=0; i<size; i+=2,j++)
			{
				seq.bytes.b0 = Buf[i+0];	//use 'seq' as temp variable to combine bytes into short int
				seq.bytes.b1 = Buf[i+1];
				pParent->m_pUdpRxQueue[pParent->m_RxQueueHead][j] = (double)seq.sall;
			}
		}
		pParent->m_RxQueueHead++;
		pParent->m_RxQueueHead &= (RXQUEUE_SIZE-1);
	}
	pParent->m_QWaitFifoData.wakeAll();
	pParent->m_UdpRxMutex.unlock();
}

#define FILE_NAME "SSB-7210000Hz_001.wav"
//&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+
// CIQDataThread class implementation.
//&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+&+
CIQDataThread::CIQDataThread(QObject* pParent) : m_pParent(pParent)
{
#if 0
	QDir::setCurrent("d:/");
	m_File.setFileName(FILE_NAME);
	if(m_File.open(QIODevice::ReadOnly))
	{
		qDebug()<<"file Opend OK";
		m_File.seek(0x7e);		//SV
	}
	else
		qDebug()<<"file Failed to Open";
#endif
}

CIQDataThread::~CIQDataThread()
{
#if 0
	if(m_File.isOpen())
	{

		m_File.close();
		qDebug()<<"file closed";
	}
#endif
}

//////////////////////////////////////////////////////////////////////////
// IQData thread waits for UDP Rx data available in Queue
// then sends it to parent via virtual function call
//////////////////////////////////////////////////////////////////////////
void CIQDataThread::run()
{
CNetIOBase* pParent;
	pParent = (CNetIOBase*)m_pParent;
	while(!pParent->m_IQDataThreadQuit)
	{
		pParent->m_UdpRxMutex.lock();
		pParent->m_QWaitFifoData.wait( &pParent->m_UdpRxMutex, 100);
		if(pParent->m_SampleSize24)
		{
			while(pParent->m_RxQueueHead != pParent->m_RxQueueTail)
			{
				pParent->ProcessIQData( &pParent->m_pUdpRxQueue[pParent->m_RxQueueTail][0], (PKT_LENGTH_24-4)/3 );
				pParent->m_RxQueueTail++;
				pParent->m_RxQueueTail &= (RXQUEUE_SIZE-1);
			}
//FileTest();
		}
		else
		{
			while(pParent->m_RxQueueHead != pParent->m_RxQueueTail)
			{
				pParent->ProcessIQData( &pParent->m_pUdpRxQueue[pParent->m_RxQueueTail][0], (PKT_LENGTH_16-4)/2 );
				pParent->m_RxQueueTail++;
				pParent->m_RxQueueTail &= (RXQUEUE_SIZE-1);
			}
		}
		pParent->m_UdpRxMutex.unlock();
	}
}


void CIQDataThread::FileTest()
{
tBtoL4 tmp;
char buf[16384];
double fBuf[1024];
CNetIOBase* pParent;
	pParent = (CNetIOBase*)m_pParent;
	while(!m_File.atEnd())
	{
		if(m_File.read(buf,3*1024)<=0)
			return;
		int j=0;
		for(int i=0; i<1024; i++)
		{
			tmp.bytes.b0 = 0;
			tmp.bytes.b1 = buf[j++];
			tmp.bytes.b2 = buf[j++];
			tmp.bytes.b3 = buf[j++];
			fBuf[i] = (double)(tmp.all/65536);
		}
		pParent->ProcessIQData( fBuf, 1024 );
	}
}
