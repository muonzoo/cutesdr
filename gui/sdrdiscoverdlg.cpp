/////////////////////////////////////////////////////////////////////
// sdrdiscoverdlg.cpp: implementation of the CSdrDiscoverDlg class.
//
//	This class implements a discover widget to find RFSPACE SDR's
//connected to a network using a simple UDP broadcast method.
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
/////////////////////////////////////////////////////////////////////

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
#include "gui/sdrdiscoverdlg.h"
#include <QDebug>


extern QUdpSocket g_UdpDiscoverSocket;

/*---------------------------------------------------------------------------*/
/*--------------------> L O C A L   D E F I N E S <--------------------------*/
/*---------------------------------------------------------------------------*/
/* UDP port numbers for Discover */
#define	DISCOVER_SERVER_PORT 48321	/* PC client Tx port, SDR Server Rx Port */
#define DISCOVER_CLIENT_PORT 48322	/* PC client Rx port, SDR Server Tx Port */

#define KEY0 0x5A
#define KEY1 0xA5
#define MSG_REQ 0
#define MSG_RESP 1
#define MSG_SET 2

//////////////////////////////////////////////////////////////////////////////
//Constructor/Destructor
//////////////////////////////////////////////////////////////////////////////
CSdrDiscoverDlg::CSdrDiscoverDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CSdrDiscoverDlg)
{
    ui->setupUi(this);
	ui->listWidget->clear();
	m_Name = "";
	m_NameFilter = "";
}

CSdrDiscoverDlg::~CSdrDiscoverDlg()
{
	m_Timer.stop();
	delete ui;
}

//Fill in initial data
void CSdrDiscoverDlg::InitDlg()
{
	OnFind();
}

//////////////////////////////////////////////////////////////////////////////
//Called when Find button pressed
//////////////////////////////////////////////////////////////////////////////
void CSdrDiscoverDlg::OnFind()
{
qint64 length;
//QT bug, can only bind once and socket cannot be deleted and re-instantiated
// so make global and bind just once
//	if(!g_UdpDiscoverSocket.bind(DISCOVER_CLIENT_PORT) )
//		return;
	ui->listWidget->clear();	//clear screen then delay so user sees it clear before getting new data
	length = sizeof(tDiscover_COMMONMSG);
	memset((void*)&m_DiscovermsgCommon[0], 0, length);
	m_DiscovermsgCommon[0].length[0] = length&0xFF; m_DiscovermsgCommon[0].length[1] = (length>>8)&0xFF;
	m_DiscovermsgCommon[0].key[0] = KEY0; m_DiscovermsgCommon[0].key[1] = KEY1;
	m_DiscovermsgCommon[0].op = MSG_REQ;
	for(int i=0; i<m_NameFilter.length(); i++)
		 m_DiscovermsgCommon[0].name[i] = m_NameFilter[i].toAscii();
	g_UdpDiscoverSocket.flush();
	g_UdpDiscoverSocket.writeDatagram( (const char*)&m_DiscovermsgCommon[0], length, QHostAddress::Broadcast, DISCOVER_SERVER_PORT);
	m_Timer.singleShot(250, this, SLOT( OnTimerRead()) );	//delay some time before trying to read responses
}

//Called by timer to read any broadcast messages then close rx port
void CSdrDiscoverDlg::OnTimerRead()
{
	if(g_UdpDiscoverSocket.hasPendingDatagrams())
		ReadUDPMessages();
}

//Called when UDP messages are received on the client port for parsing
void CSdrDiscoverDlg::ReadUDPMessages()
{
qint64 totallength;
qint64 length;
int index=0;
char Buf[2048];	//buffer to hold received UDP packet
	do
	{	//loop and get all UDP packets availaable
		index = ui->listWidget->count();
		totallength = g_UdpDiscoverSocket.pendingDatagramSize();
		g_UdpDiscoverSocket.readDatagram( Buf, totallength);	//read entire UDP packet
		//copy just the common fixed size block into m_DiscovermsgCommon
		length = sizeof(tDiscover_COMMONMSG);
		memcpy((void*)&m_DiscovermsgCommon[index], (void*)Buf, length );
		if( (KEY0 == m_DiscovermsgCommon[index].key[0]) && (KEY1 == m_DiscovermsgCommon[index].key[1]) && (index<MAX_DEVICES) )
		{
			quint16 tmp16 = m_DiscovermsgCommon[index].port[1]; tmp16 <<= 8; tmp16 |= m_DiscovermsgCommon[index].port[0];
			QString str = QString("%1   SN=%2   IP=%3.%4.%5.%6   Port=%7")
					.arg(m_DiscovermsgCommon[index].name)
					.arg(m_DiscovermsgCommon[index].sn)
					.arg(m_DiscovermsgCommon[index].ipaddr[3])
					.arg(m_DiscovermsgCommon[index].ipaddr[2])
					.arg(m_DiscovermsgCommon[index].ipaddr[1])
					.arg(m_DiscovermsgCommon[index].ipaddr[0])
					.arg(tmp16);
			ui->listWidget->addItem(str);	//place formated information in listbox
		}
	}while( g_UdpDiscoverSocket.hasPendingDatagrams() );
}

//////////////////////////////////////////////////////////////////////////////
//Called when someone double clicks on a listbox entry for viewing/editing
//////////////////////////////////////////////////////////////////////////////
void CSdrDiscoverDlg::OnItemDoubleClick( QListWidgetItem * item )
{
	Q_UNUSED(item);
	accept();
}


void CSdrDiscoverDlg::accept()
{	//OK was pressed so get all data from edit controls
int index = ui->listWidget->currentRow();
	if(index>=0)
	{
		m_Name = m_DiscovermsgCommon[index].name;
		m_Port = m_DiscovermsgCommon[index].port[1];
		m_Port <<= 8;
		m_Port |= m_DiscovermsgCommon[index].port[0];
		m_IPAdr = m_DiscovermsgCommon[index].ipaddr[3];
		m_IPAdr<<=8;
		m_IPAdr += m_DiscovermsgCommon[index].ipaddr[2];
		m_IPAdr<<=8;
		m_IPAdr += m_DiscovermsgCommon[index].ipaddr[1];
		m_IPAdr<<=8;
		m_IPAdr += m_DiscovermsgCommon[index].ipaddr[0];
	}
	 else
	{
		 m_Name = "";
	}
	QDialog::accept();		//call base class to exit
}
