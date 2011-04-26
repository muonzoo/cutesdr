/////////////////////////////////////////////////////////////////////
// sdrinterface.cpp: implementation of the CSdrInterface class.
//
//	This class implements the interface between the upper GUI and
//the specific SDR.  It parses messages/data received by the radio
// and sends messages to the radio for setup and control.
//
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
//	2011-04-16  Added Frequency range logic for optional down converter modules
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
#include "interface/sdrinterface.h"
#include "gui/testbench.h"
#include <QDebug>

#define SPUR_CAL_MAXSAMPLES 300000
#define MAX_SAMPLERATES 4

//Tables to get various parameters based on the gui sdrsetup samplerate index value
const quint32 SDRIQ_MAXBW[MAX_SAMPLERATES] =
{
	50000,
	100000,
	150000,
	190000
};

const quint32 SDRIQ_6620FILTERS[MAX_SAMPLERATES] =
{
	Cad6620::BWKHZ_50,
	Cad6620::BWKHZ_100,
	Cad6620::BWKHZ_150,
	Cad6620::BWKHZ_190
};

const double SDRIQ_6620FILTERGAIN[MAX_SAMPLERATES] =
{
	0.0,
	8.0,
	11.0,
	22.0
};

const double SDRIQ_SAMPLERATE[MAX_SAMPLERATES] =
{
	(66666666.6667/1200.0),
	(66666666.6667/600.0),
	(66666666.6667/420.0),
	(66666666.6667/340.0)
};

const quint32 NETSDR_MAXBW[MAX_SAMPLERATES] =
{
	50000,
//	100000,
	200000,
	500000,
	1600000
};

const double NETSDR_SAMPLERATE[MAX_SAMPLERATES] =
{
	(80.0e6/1280.0),
	(80.0e6/320.0),
	(80.0e6/128.0),
	(80.0e6/40.0)
};

const quint32 SDRIP_MAXBW[MAX_SAMPLERATES] =
{
	50000,
	200000,
	500000,
	1800000
};

const double SDRIP_SAMPLERATE[MAX_SAMPLERATES] =
{
	(80.0e6/1280.0),
	(80.0e6/320.0),
	(80.0e6/130.0),
	(80.0e6/40.0)
};


/////////////////////////////////////////////////////////////////////
// Constructor/Destructor
/////////////////////////////////////////////////////////////////////
CSdrInterface::CSdrInterface()
{
	m_Running = false;
	m_BootRev = 0.0;
	m_AppRev = 0.0;
	m_FftBufPos = 0;
	m_RfGain = 0;
	m_KeepAliveCounter = 0;
	m_GainCalibrationOffset = 0.0;
	m_SampleRate = 1.0;
	m_MaxBandwidth = 1;
	m_SerialNum = "";
	m_DeviceName = "";
	m_BandwidthIndex = -1;
	m_RadioType = SDR14;
	m_FftSize = 4096;
	m_DisplaySkipCounter = 0;
	m_NCOSpurOffsetI = 0.0;
	m_NCOSpurOffsetQ = 0.0;
	m_MaxDisplayRate = 10;
	m_CurrentFrequency = 0;
	m_BaseFrequencyRangeMin = 0;		//load default frequency ranges
	m_BaseFrequencyRangeMax = 30000000;
	m_OptionFrequencyRangeMin = 0;
	m_OptionFrequencyRangeMax = 30000000;
	SetMaxDisplayRate(m_MaxDisplayRate);
	m_ScreenUpateFinished = TRUE;
	SetFftSize(4096);
	SetFftAve(1);
	m_pSoundCardOut = new CSoundOut(this);
	m_Status = NOT_CONNECTED;
	m_ChannelMode = CI_RX_CHAN_SETUP_SINGLE_1;	//default channel settings for NetSDR
	m_Channel = CI_RX_CHAN_1;
}

CSdrInterface::~CSdrInterface()
{
	if(m_pSoundCardOut)
		delete m_pSoundCardOut;
}

/////////////////////////////////////////////////////////////////////
// returns the maximum bandwidth based on radio and gui bw index
/////////////////////////////////////////////////////////////////////
qint32 CSdrInterface::GetMaxBWFromIndex(qint32 index)
{
qint32 ret = 0;
if(index >= MAX_SAMPLERATES)
	return 100000;
	switch(m_RadioType)
	{
		case SDR14:
		case SDRIQ:
			ret = SDRIQ_MAXBW[index];
			break;
		case SDRIP:
			ret = SDRIP_MAXBW[index];
			break;
		case NETSDR:
			ret = NETSDR_MAXBW[index];
			break;
		default:
			break;
	}
	return ret;
}

/////////////////////////////////////////////////////////////////////
// returns the maximum bandwidth based on radio and gui bw index
/////////////////////////////////////////////////////////////////////
double CSdrInterface::GetSampleRateFromIndex(qint32 index)
{
double ret = 1.0;
	if(index >= MAX_SAMPLERATES)
		return 100000;
	switch(m_RadioType)
	{
		case SDR14:
		case SDRIQ:
			ret = SDRIQ_SAMPLERATE[index];
			break;
		case SDRIP:
			ret = SDRIP_SAMPLERATE[index];
			break;
		case NETSDR:
			ret = NETSDR_SAMPLERATE[index];
			break;
		default:
			break;
	}
	return ret;
}

/////////////////////////////////////////////////////////////////////
// called to start up io device threads and open device.
/////////////////////////////////////////////////////////////////////
void CSdrInterface::StartIO()
{
	CNetIOBase::StartIO();
}

/////////////////////////////////////////////////////////////////////
// called to stop io device threads and close device.
/////////////////////////////////////////////////////////////////////
void CSdrInterface::StopIO()
{
	StopSdr();
	CNetIOBase::StopIO();
}

///////////////////////////////////////////////////////////////////////////////
// sends signal to indicate network/radio connection status
///////////////////////////////////////////////////////////////////////////////
void CSdrInterface::SendIOStatus(int iostatus)
{
	m_Status = (eStatus)iostatus;
	emit NewStatus( iostatus );
}

////////////////////////////////////////////////////////////////////////
//  Called from worker thread with new ASCP message to parse from radio
//  Cannot call any QT functions since is a thread so use signals
// to inform the GUI.
////////////////////////////////////////////////////////////////////////
void CSdrInterface::ParseAscpMsg(CAscpMsg *pMsg)
{
quint16 Length;
quint32 tmp32;
	pMsg->InitRxMsg();	//initialize receive msg object for read back
	if( pMsg->GetType() == TYPE_TARG_RESP_CITEM )
	{	// Is a message from SDR in response to a request
//qDebug()<<"Msg "<<pMsg->GetCItem();
		switch(pMsg->GetCItem())
		{
			case CI_GENERAL_INTERFACE_NAME:
				m_DeviceName = (const char*)(&pMsg->Buf8[4]);
				// create radio type value from connected device string
				if("SDR-14" == m_DeviceName)
					m_RadioType = SDR14;
				else if("SDR-IQ" == m_DeviceName)
					m_RadioType = SDRIQ;
				else if("SDR-IP" == m_DeviceName)
					m_RadioType = SDRIP;
				if("NetSDR" == m_DeviceName)
					m_RadioType = NETSDR;
				break;
			case CI_GENERAL_INTERFACE_SERIALNUM:
				m_SerialNum = (const char*)(&pMsg->Buf8[4]);
				break;
			case CI_GENERAL_INTERFACE_VERSION:
				break;
			case CI_GENERAL_HARDFIRM_VERSION:
				if(pMsg->GetParm8() == 0)
				{
					m_BootRev = (float)pMsg->GetParm16()/100.0;
				}
				else
				{
					m_AppRev =(float)pMsg->GetParm16()/100.0;
					m_BandwidthIndex = -1;	//force update of sample rate logic
					emit NewInfoData();
				}
				break;
			case CI_RX_STATE:
				pMsg->GetParm8();
				if(RX_STATE_ON == pMsg->GetParm8())
				{
					emit NewStatus( RUNNING );
					m_Running = true;
				}
				else
				{
					emit NewStatus( CONNECTED );
					m_Running = false;
				}
				break;
			case CI_GENERAL_OPTIONS:
				break;
			case CI_GENERAL_SECURITY_CODE:
//qDebug()<<"security = "<<pMsg->GetParm16();
				break;
			case CI_GENERAL_STATUS_CODE:	//used as keepalive ack
				m_KeepAliveCounter = 0;
				break;
			case CI_RX_FREQUENCY:
				pMsg->GetParm8();
				tmp32 = pMsg->GetParm32();
				break;
			case CI_RX_OUT_SAMPLE_RATE:
				pMsg->GetParm8();
				m_SampleRate = (double)pMsg->GetParm32();
				break;
			case CI_GENERAL_PRODUCT_ID:
				break;
			case CI_UPDATE_MODE_PARAMS:
				break;
			default:
				break;
		}
	}
	else if( pMsg->GetType() == TYPE_TARG_RESP_CITEM_RANGE )
	{	// Is a range message from SDR
		switch( pMsg->GetCItem() )
		{
			case CI_RX_FREQUENCY:
				Length = pMsg->GetLength();
				pMsg->GetParm8();
				m_BaseFrequencyRangeMin = (quint64)pMsg->GetParm32();
				pMsg->GetParm8();
				m_BaseFrequencyRangeMax = (quint64)pMsg->GetParm32();
				pMsg->GetParm8();
				m_OptionFrequencyRangeMin = m_BaseFrequencyRangeMin;	//set option range to base range
				m_OptionFrequencyRangeMax = m_BaseFrequencyRangeMax;
				if(Length>15)
				{
					m_OptionFrequencyRangeMin = (quint64)pMsg->GetParm32();
					pMsg->GetParm8();
					m_OptionFrequencyRangeMax = (quint64)pMsg->GetParm32();
				}
//qDebug()<<"Base range"<<m_BaseFrequencyRangeMin << m_BaseFrequencyRangeMax;
//qDebug()<<"Option range"<<m_OptionFrequencyRangeMin << m_OptionFrequencyRangeMax;
				break;
			default:
				break;
		}
	}
	else if( pMsg->GetType() == TYPE_TARG_UNSOLICITED_CITEM )
	{	// Is an unsolicited message from SDR
		switch( pMsg->GetCItem() )
		{
			case CI_GENERAL_STATUS_CODE:
				if( GENERAL_STATUS_ADOVERLOAD == pMsg->GetParm8() )
					SendIOStatus(ADOVR);
				break;
			default:
				break;
		}
	}
	else if( pMsg->GetType() == TYPE_TARG_DATA_ITEM0 )
	{
	}
	else if( pMsg->GetType() == TYPE_TARG_DATA_ITEM1 )
	{
	}
	else if( pMsg->GetType() == TYPE_TARG_DATA_ITEM2 )
	{
	}
	else if( pMsg->GetType() == TYPE_TARG_DATA_ITEM3 )
	{
	}
	else if(pMsg->GetType() == TYPE_DATA_ITEM_ACK)
	{
		switch(pMsg->Buf8[2])
		{	//decode Data acks
			case 0:		//NetSDR and SDRIP keepalive ack
				break;
			case 1: 	//ack of AD6620 load so ok to send next msg if any left to send
				if( m_AD6620.GetNext6620Msg(m_TxMsg) )
					SendAscpMsg(&m_TxMsg);
				else
					qDebug()<<"AD6620 Load Complete";
				break;
			case 2:
				break;
			case 3:
				break;
		}
	}
}

/////////////////////////////////////////////////////////////////////
// Called to ack a data item
/////////////////////////////////////////////////////////////////////
void CSdrInterface::SendAck(quint8 chan)
{
	m_TxMsg.InitTxMsg(TYPE_DATA_ITEM_ACK);
	m_TxMsg.AddParm8(chan);
	SendAscpMsg(&m_TxMsg);
}

void CSdrInterface::SetRx2Parameters(double Rx2Gain, double Rx2Phase)
{
quint16 gain;
quint32 phase;
//	qDebug()<<"Rx2Gain = "<<Rx2Gain <<" Rx2Phase = "<<Rx2Phase;
	gain = (quint16)(Rx2Gain*32767.0);

	m_TxMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
	m_TxMsg.AddCItem(CI_RX_ADCGAIN);
	m_TxMsg.AddParm8(CI_RX_CHAN_2);
	m_TxMsg.AddParm16(0x7FFF);
	SendAscpMsg(&m_TxMsg);

	m_TxMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
	m_TxMsg.AddCItem(CI_RX_ADCGAIN);
	m_TxMsg.AddParm8(CI_RX_CHAN_1);
	m_TxMsg.AddParm16(gain);
	SendAscpMsg(&m_TxMsg);


	phase = (quint32)( (Rx2Phase/360.0) * 4294967295.0);

	m_TxMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
	m_TxMsg.AddCItem(CI_RX_NCOPHASE);
	m_TxMsg.AddParm8(CI_RX_CHAN_1);
	m_TxMsg.AddParm32(0);
	SendAscpMsg(&m_TxMsg);

	m_TxMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
	m_TxMsg.AddCItem(CI_RX_NCOPHASE);
	m_TxMsg.AddParm8(CI_RX_CHAN_2);
	m_TxMsg.AddParm32(phase);
	SendAscpMsg(&m_TxMsg);

	qDebug()<<"Gain = "<<gain <<" Rx2Phase = "<<phase;
}

/////////////////////////////////////////////////////////////////////
// Called to request a bunch of general information from the SDR
/////////////////////////////////////////////////////////////////////
void CSdrInterface::GetSdrInfo()
{
	m_TxMsg.InitTxMsg(TYPE_HOST_REQ_CITEM);
	m_TxMsg.AddCItem(CI_GENERAL_INTERFACE_NAME);
	SendAscpMsg(&m_TxMsg);

	m_TxMsg.InitTxMsg(TYPE_HOST_REQ_CITEM);
	m_TxMsg.AddCItem(CI_GENERAL_INTERFACE_SERIALNUM);
	SendAscpMsg(&m_TxMsg);

	m_TxMsg.InitTxMsg(TYPE_HOST_REQ_CITEM);
	m_TxMsg.AddCItem(CI_GENERAL_HARDFIRM_VERSION);
	m_TxMsg.AddParm8(0);
	SendAscpMsg(&m_TxMsg);

	m_TxMsg.InitTxMsg(TYPE_HOST_REQ_CITEM);
	m_TxMsg.AddCItem(CI_GENERAL_HARDFIRM_VERSION);
	m_TxMsg.AddParm8(1);
	SendAscpMsg(&m_TxMsg);

	if( (SDRIP==m_RadioType) || (NETSDR==m_RadioType) )
	{
		m_TxMsg.InitTxMsg(TYPE_HOST_REQ_CITEM_RANGE);
		m_TxMsg.AddCItem(CI_RX_FREQUENCY);
		m_TxMsg.AddParm8(CI_RX_CHAN_1);
		SendAscpMsg(&m_TxMsg);
	}
}


/////////////////////////////////////////////////////////////////////
// Called to request sdr status
/////////////////////////////////////////////////////////////////////
void CSdrInterface::ReqStatus()
{
	m_TxMsg.InitTxMsg(TYPE_HOST_REQ_CITEM);
	m_TxMsg.AddCItem(CI_GENERAL_STATUS_CODE);
	SendAscpMsg(&m_TxMsg);
}


/////////////////////////////////////////////////////////////////////
// Called to set the channel mode
/////////////////////////////////////////////////////////////////////
void CSdrInterface::SetChannelMode(qint32 channelmode)
{
	m_ChannelMode = channelmode;
	switch(m_ChannelMode)
	{
		case CI_RX_CHAN_SETUP_SINGLE_1:
			m_Channel = CI_RX_CHAN_1;
			break;
		case CI_RX_CHAN_SETUP_SINGLE_2:
			m_Channel = CI_RX_CHAN_2;
			break;
		case CI_RX_CHAN_SETUP_SINGLE_SUM:
			m_Channel = CI_RX_CHAN_ALL;

			break;
		case CI_RX_CHAN_SETUP_SINGLE_DIF:
			m_Channel = CI_RX_CHAN_ALL;
			break;
		default:
			break;
	}
}

/////////////////////////////////////////////////////////////////////
// Called to start the SDR
/////////////////////////////////////////////////////////////////////
void CSdrInterface::StartSdr()
{
	m_FftBufPos = 0;

	switch(m_RadioType)
	{
		case SDRIP:
		case NETSDR:
			emit NewStatus( RUNNING );

			m_TxMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
			m_TxMsg.AddCItem(CI_RX_CHAN_SETUP);
			m_TxMsg.AddParm8(m_ChannelMode);
			SendAscpMsg(&m_TxMsg);

			m_TxMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
			m_TxMsg.AddCItem(CI_RX_RF_FILTER);
			m_TxMsg.AddParm8(m_Channel);
			m_TxMsg.AddParm8(CI_RX_RF_FILTER_AUTO);
			SendAscpMsg(&m_TxMsg);

			m_TxMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
			m_TxMsg.AddCItem(CI_RX_AD_MODES);
			m_TxMsg.AddParm8(m_Channel);
			m_TxMsg.AddParm8(  CI_AD_MODES_DITHER | CI_AD_MODES_PGA);
			SendAscpMsg(&m_TxMsg);

			m_TxMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
			m_TxMsg.AddCItem(CI_RX_SYNCIN_MODE_PARAMETERS);
			m_TxMsg.AddParm8(0);
			m_TxMsg.AddParm8(CI_RX_SYNCIN_MODE_OFF);
			SendAscpMsg(&m_TxMsg);

			m_TxMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
			m_TxMsg.AddCItem(CI_RX_PULSEOUT_MODE);
			m_TxMsg.AddParm8(0);
			m_TxMsg.AddParm8(CI_PULSEOUT_MODE_OFF);
			SendAscpMsg(&m_TxMsg);

			m_TxMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
			m_TxMsg.AddCItem(CI_RX_OUT_SAMPLE_RATE);
			m_TxMsg.AddParm8(0);
			m_TxMsg.AddParm32((quint32)m_SampleRate);
			SendAscpMsg(&m_TxMsg);

			m_TxMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
			m_TxMsg.AddCItem(CI_RX_STATE);
			m_TxMsg.AddParm8(RX_STATE_DATACOMPLEX);
			m_TxMsg.AddParm8(RX_STATE_ON);
			if(m_SampleRate<1500000.0)
				m_TxMsg.AddParm8(MODE_CONTIGUOUS24);
			else
				m_TxMsg.AddParm8(MODE_CONTIGUOUS16);
			m_TxMsg.AddParm8(0);
			SendAscpMsg(&m_TxMsg);

			m_NCOSpurOffsetI = 0.0;		//don't need for netsdr
			m_NCOSpurOffsetQ = 0.0;
			m_NcoSpurCalActive = FALSE;
			break;
		case SDR14:
		case SDRIQ:
			emit NewStatus( RUNNING );

			m_TxMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
			m_TxMsg.AddCItem(CI_RX_IF_GAIN);
			m_TxMsg.AddParm8(0);
			m_TxMsg.AddParm32(24);
			SendAscpMsg(&m_TxMsg);

			m_TxMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
			m_TxMsg.AddCItem(CI_RX_STATE);
			m_TxMsg.AddParm8(RX_STATE_COMPLEX_HF);
			m_TxMsg.AddParm8(RX_STATE_ON);
			m_TxMsg.AddParm8(MODE_CONTIGUOUS16);
			m_TxMsg.AddParm8(0);
			SendAscpMsg(&m_TxMsg);
			ManageNCOSpurOffsets(CSdrInterface::NCOSPUR_CMD_STARTCAL,0,0);
			break;
	}
	SetSdrRfGain(m_RfGain);
	m_ScreenUpateFinished = TRUE;
	m_KeepAliveCounter = 0;
	//setup and start soundcard output
	if(!m_pSoundCardOut->Start(m_SoundOutIndex, m_StereoOut, m_Demodulator.GetOutputRate(), false) )
		SendIOStatus(ERROR);
//qDebug()<<"SR="<<m_SampleRate;
}

///////////////////////////////////////////////////////////////////////////////
// Sends Stop command to SDR
///////////////////////////////////////////////////////////////////////////////
void CSdrInterface::StopSdr()
{
	m_Running = false;
	m_pSoundCardOut->Stop();
	m_TxMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
	m_TxMsg.AddCItem(CI_RX_STATE);
	m_TxMsg.AddParm8(RX_STATE_DATACOMPLEX);
	m_TxMsg.AddParm8(RX_STATE_IDLE);
	m_TxMsg.AddParm8(0);
	m_TxMsg.AddParm8(0);
	SendAscpMsg(&m_TxMsg);
}

///////////////////////////////////////////////////////////////////////////////
// Sends RF gain command to SDR
///////////////////////////////////////////////////////////////////////////////
void CSdrInterface::SetSdrRfGain(qint32 gain)
{
	m_RfGain = gain;
	m_TxMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
	m_TxMsg.AddCItem(CI_RX_RF_GAIN);
	m_TxMsg.AddParm8(m_Channel);
	m_TxMsg.AddParm8((qint8)gain);
	SendAscpMsg(&m_TxMsg);
//qDebug()<<"gain "<<gain;
	//try to keep dB calibration close to dBm at the radio antenna connector
	switch(m_RadioType)
	{	//  *** todo: needs to allow user calibration data  ***
		case SDRIP:
			m_GainCalibrationOffset = -10.0;
			break;
		case NETSDR:
			m_GainCalibrationOffset = -12.0;
			break;
		case SDR14:
			m_GainCalibrationOffset = -49.0 + SDRIQ_6620FILTERGAIN[m_BandwidthIndex];
			break;
		case SDRIQ:
			m_GainCalibrationOffset = -49.0 + SDRIQ_6620FILTERGAIN[m_BandwidthIndex];
			break;
	}
	m_Fft.SetFFTParams( m_FftSize,
						FALSE,
						m_GainCalibrationOffset-m_RfGain,
						m_SampleRate);
}

///////////////////////////////////////////////////////////////////////////////
// Sends Center frequency command to SDR
///////////////////////////////////////////////////////////////////////////////
quint64 CSdrInterface::SetRxFreq(quint64 freq)
{
	if(NOT_CONNECTED == m_Status)	//just return if not conencted to sdr
		return freq;

	//clamp to range of receiver and any options
//qDebug()<<"F = "<<freq;
	if(freq > m_OptionFrequencyRangeMax)	//if greater than max range
		freq = m_OptionFrequencyRangeMax;
	if(	(freq > m_BaseFrequencyRangeMax) && ( freq < m_OptionFrequencyRangeMin) )	//if in invalid region
	{
		if(	freq > m_CurrentFrequency)	//if last freq lower then go to converter range bottom
			freq = m_OptionFrequencyRangeMin;
		else
			freq = m_BaseFrequencyRangeMax; //else last freq higher then go to base range top
	}
	m_CurrentFrequency = freq;

	m_TxMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
	m_TxMsg.AddCItem(CI_RX_FREQUENCY);
	m_TxMsg.AddParm8(m_Channel);
	m_TxMsg.AddParm32( (quint32)freq );
	m_TxMsg.AddParm8(0);
	SendAscpMsg(&m_TxMsg);

	if(SDRIP==m_RadioType)
	{
		m_TxMsg.InitTxMsg(TYPE_HOST_SET_CITEM);
		m_TxMsg.AddCItem(CI_RX_FREQUENCY);
		m_TxMsg.AddParm8(CI_RX_FREQUENCY_DISPLAY);
		m_TxMsg.AddParm32( (quint32)freq );
		m_TxMsg.AddParm8(0);
		SendAscpMsg(&m_TxMsg);
	}
	return freq;
}

///////////////////////////////////////////////////////////////////////////////
// Send Keep alive message to SDR
///////////////////////////////////////////////////////////////////////////////
void CSdrInterface::KeepAlive()
{
	m_TxMsg.InitTxMsg(TYPE_HOST_REQ_CITEM);
	m_TxMsg.AddCItem(CI_GENERAL_STATUS_CODE);
	SendAscpMsg(&m_TxMsg);
	if(++m_KeepAliveCounter >2)		//see if no ack received
	{
		SendIOStatus(ERROR);
qDebug()<<"Keepalive failed";
		m_KeepAliveCounter = 0;
	}
}


///////////////////////////////////////////////////////////////////////////////
//Set display FFT size(must be power of 2)
///////////////////////////////////////////////////////////////////////////////
void CSdrInterface::SetFftSize(qint32 size)
{
	m_FftSize = size;
	m_FftBufPos = 0;
	m_Fft.SetFFTParams( m_FftSize,
						FALSE,
						m_GainCalibrationOffset,
						m_SampleRate);
	SetMaxDisplayRate(m_MaxDisplayRate);
}

///////////////////////////////////////////////////////////////////////////////
//called to change SDR sample rate based on the GUI index value(0-3)
///////////////////////////////////////////////////////////////////////////////
void CSdrInterface::SetSdrBandwidthIndex(qint32 bwindex)
{
	if(m_BandwidthIndex == bwindex)
		return;		//if nothing changed
	m_BandwidthIndex = bwindex;
	StopSdr();
	switch(m_RadioType)
	{
		case SDR14:
		case SDRIQ:		//need to load AD6620 parameters
			m_SampleRate = SDRIQ_SAMPLERATE[m_BandwidthIndex];
			m_MaxBandwidth = SDRIQ_6620FILTERS[m_BandwidthIndex];
			//SDR-IQ/14 requires filter change to set sample rate
			m_AD6620.CreateLoad6620Msgs(SDRIQ_6620FILTERS[m_BandwidthIndex]);
			if( m_AD6620.GetNext6620Msg(m_TxMsg) )
			{
				SendAscpMsg(&m_TxMsg);
			}
			break;
		case SDRIP:
			m_SampleRate = SDRIP_SAMPLERATE[m_BandwidthIndex];
			m_MaxBandwidth = SDRIP_MAXBW[m_BandwidthIndex];
			break;
		case NETSDR:
			m_SampleRate = NETSDR_SAMPLERATE[m_BandwidthIndex];
			m_MaxBandwidth = NETSDR_MAXBW[m_BandwidthIndex];
			break;
	}
	SetFftSize(m_FftSize);	//need to tell fft because sample rate has changed
	SetMaxDisplayRate(m_MaxDisplayRate);
	m_Demodulator.SetInputSampleRate(m_SampleRate);
	m_pSoundCardOut->ChangeUserDataRate( m_Demodulator.GetOutputRate());
qDebug()<<"UsrDataRate="<< m_Demodulator.GetOutputRate();
}

///////////////////////////////////////////////////////////////////////////////
//called to change demodulation parameters
///////////////////////////////////////////////////////////////////////////////
void CSdrInterface::SetDemod(int Mode, tDemodInfo CurrentDemodInfo)
{
	m_Demodulator.SetDemod(Mode, CurrentDemodInfo );
	m_pSoundCardOut->ChangeUserDataRate( m_Demodulator.GetOutputRate());
}

///////////////////////////////////////////////////////////////////////////////
//called to change Noise Processing parameters
///////////////////////////////////////////////////////////////////////////////
void CSdrInterface::SetupNoiseProc(tNoiseProcdInfo* pNoiseProcSettings)
{
	m_NoiseProc.SetupBlanker( pNoiseProcSettings->NBOn,
							  pNoiseProcSettings->NBThreshold,
							  pNoiseProcSettings->NBWidth,
							  m_SampleRate);
}


///////////////////////////////////////////////////////////////////////////////
//Set Display FFT average value
///////////////////////////////////////////////////////////////////////////////
void CSdrInterface::SetFftAve(qint32 ave)
{
	m_FftAve = ave;
	m_Fft.SetFFTAve(ave);
}

////////////////////////////////////////////////////////////////////////
// Called to read/set/start calibration of the NCO Spur Offset value.
////////////////////////////////////////////////////////////////////////
void CSdrInterface::ManageNCOSpurOffsets( eNCOSPURCMD cmd, double* pNCONullValueI,  double* pNCONullValueQ)
{
	m_NcoSpurCalActive = FALSE;
	switch(cmd)
	{
		case NCOSPUR_CMD_SET:
			if( (NULL!=pNCONullValueI) && (NULL!=pNCONullValueQ) )
			{
				m_NCOSpurOffsetI = *pNCONullValueI;
				m_NCOSpurOffsetQ = *pNCONullValueQ;
//qDebug()<<"Cal Set"<< m_NCOSpurOffsetI << m_NCOSpurOffsetQ;
			}
			break;
		case NCOSPUR_CMD_STARTCAL:
			if((m_NCOSpurOffsetI>10.0) || (m_NCOSpurOffsetI<-10.0))
				m_NCOSpurOffsetI = 0.0;
			if((m_NCOSpurOffsetQ>10.0) || (m_NCOSpurOffsetQ<-10.0))
				m_NCOSpurOffsetQ = 0.0;
			m_NcoSpurCalCount = 0;
			m_NcoSpurCalActive = TRUE;
//qDebug()<<"Start NCO Cal";
			break;
		case NCOSPUR_CMD_READ:
			if( (NULL!=pNCONullValueI) && (NULL!=pNCONullValueQ) )
			{
				*pNCONullValueI = m_NCOSpurOffsetI;
				*pNCONullValueQ = m_NCOSpurOffsetQ;
//qDebug()<<"Cal Read"<< m_NCOSpurOffsetI << m_NCOSpurOffsetQ;
			}
			break;
		default:
			break;
	}
}

////////////////////////////////////////////////////////////////////////
// Called to calculate the NCO Spur Offset value from the incoming m_DataBuf.
////////////////////////////////////////////////////////////////////////
void CSdrInterface::NcoSpurCalibrate(double* pData, qint32 length)
{
	if( m_NcoSpurCalCount < SPUR_CAL_MAXSAMPLES)
	{
		for( int i=0; i<length; i++)
		{	//calculate average of I and Q data to get individual DC offsets
			double tmp = pData[i];
			if(i&1)
				m_NCOSpurOffsetQ = (1.0-1.0/100000.0)*m_NCOSpurOffsetQ + (1.0/100000.0)*tmp;
			else
				m_NCOSpurOffsetI = (1.0-1.0/100000.0)*m_NCOSpurOffsetI + (1.0/100000.0)*tmp;
		}
		m_NcoSpurCalCount += (length/4);
	}
	else
	{
		m_NcoSpurCalActive = FALSE;
qDebug()<<"NCO Cal Done";
	}
}


///////////////////////////////////////////////////////////////////////////////
// Get FFT data formated for the GUI screen display.  Call it after getting
// the NewFftData() signal.
///////////////////////////////////////////////////////////////////////////////
bool CSdrInterface::GetScreenIntegerFFTData(qint32 MaxHeight, qint32 MaxWidth,
								double MaxdB, double MindB,
								qint32 StartFreq, qint32 StopFreq,
								qint32* OutBuf )
{
	return m_Fft.GetScreenIntegerFFTData( MaxHeight,
								   MaxWidth,
								  MaxdB,
								  MindB,
								  StartFreq,
								  StopFreq,
								  OutBuf);

}

///////////////////////////////////////////////////////////////////////////////
// Called by worker thread with new I/Q data fom the SDR.
//  This thread is what is used to perform all the DSP functions
// pIQData is ptr to complex I/Q double samples.  (order is I then Q)
// Length is the number of doubles in pIQData. (2x the number of data samples)
// emits "NewFftData()" when it accumulates an entire FFT length of samples
// and the display update time is ready
///////////////////////////////////////////////////////////////////////////////
void CSdrInterface::ProcessIQData( double* pIQData, int Length)
{
	if(!m_Running)	//ignor any incoming data if not running
		return;

	g_pTestBench->CreateGeneratorSamples(Length/2, (TYPECPX*)pIQData, m_SampleRate);
	m_NoiseProc.ProcessBlanker(Length/2, (TYPECPX*)pIQData, (TYPECPX*)pIQData);

	if(m_NcoSpurCalActive)	//if performing NCO spur calibration
		NcoSpurCalibrate(pIQData, Length);
	//accumulate samples into m_DataBuf until have enough to perform an FFT
	for(int i=0; i<Length; i++)
	{
		if(m_FftBufPos&1)	//apply I/Q DC offset correction to all samples
			m_DataBuf[m_FftBufPos++] = pIQData[i] - m_NCOSpurOffsetQ;
		else
			m_DataBuf[m_FftBufPos++] = pIQData[i] - m_NCOSpurOffsetI;
		if(m_FftBufPos >= (m_FftSize*2) )
		{
			m_FftBufPos = 0;
			if(++m_DisplaySkipCounter >= m_DisplaySkipValue )
			{
				m_DisplaySkipCounter = 0;
				if(m_ScreenUpateFinished)
				{
					m_Fft.PutInDisplayFFT(m_FftSize, (TYPECPX*)m_DataBuf);
					m_ScreenUpateFinished = FALSE;
					emit NewFftData();
				}
			}
		}
	}
	TYPECPX SoundBuf[8192];
	int n;
	if(m_StereoOut)
	{
		n = m_Demodulator.ProcessData(Length/2, (TYPECPX*)pIQData, SoundBuf);
		m_pSoundCardOut->PutOutQueue(n, SoundBuf);
	}
	else
	{
		n = m_Demodulator.ProcessData(Length/2, (TYPECPX*)pIQData, (TYPEREAL*)SoundBuf);
		m_pSoundCardOut->PutOutQueue(n, (TYPEREAL*)SoundBuf);
	}
}
