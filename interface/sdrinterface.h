//////////////////////////////////////////////////////////////////////
// sdrinterface.h: interface for the CSdrInterface class.
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
//	2011-04-16  Added Frequency range logic for optional down converter modules
/////////////////////////////////////////////////////////////////////
#ifndef SDRINTERFACE_H
#define SDRINTERFACE_H

#include "interface/netiobase.h"
#include "dsp/fft.h"
#include "interface/ad6620.h"
#include "dsp/demodulator.h"
#include "dsp/noiseproc.h"
#include "interface/soundout.h"
#include "interface/protocoldefs.h"


/////////////////////////////////////////////////////////////////////
// Derived class from NetIOBase for all the custom SDR msg processing
/////////////////////////////////////////////////////////////////////
class CSdrInterface : public CNetIOBase
{
	Q_OBJECT
public:
	CSdrInterface();
	~CSdrInterface();

	enum eStatus {
		NOT_CONNECTED,
		CONNECTED,
		RUNNING,
		ADOVR,
		ERROR
	};
	enum eRadioType {
		SDR14,
		SDRIQ,
		SDRIP,
		NETSDR
	};
	//NCO spur management commands for ManageNCOSpurOffsets(...)
	enum eNCOSPURCMD {
		NCOSPUR_CMD_SET,
		NCOSPUR_CMD_STARTCAL,
		NCOSPUR_CMD_READ
	};

	void StartIO();	//starts IO threads
	void StopIO();	//stops IO threads

	// virtual function called by worker thread with complete ascp rx msg to decode
	virtual void SendIOStatus(int iostatus);
	//called by TCP thread with new msg from radio to parse
	void ParseAscpMsg(CAscpMsg *pMsg);
	//called by IQData thread with new I/Q data to process
	virtual void ProcessIQData( double* pIQData, int Length);

	void StartSdr();
	void StopSdr();
	bool GetScreenIntegerFFTData(qint32 MaxHeight, qint32 MaxWidth,
									double MaxdB, double MindB,
									qint32 StartFreq, qint32 StopFreq,
									qint32* OutBuf );
	void ScreenUpdateDone(){m_ScreenUpateFinished = TRUE;}
	void KeepAlive();
	void ManageNCOSpurOffsets( eNCOSPURCMD cmd, double* pNCONullValueI,  double* pNCONullValueQ);
	void SetRx2Parameters(double Rx2Gain, double Rx2Phase);

	//calls to these functions promt a signal in response
	void GetSdrInfo();
	void ReqStatus();
	quint64 SetRxFreq(quint64 freq);

	//bunch of public members containing sdr related information and data
	QString m_DeviceName;
	QString m_SerialNum;
	float m_BootRev;
	float m_AppRev;

	//GUI Public settings and access functions
	void SetRadioType(qint32 radiotype ){m_RadioType = (eRadioType)radiotype;}
	qint32 GetRadioType(){return (qint32)m_RadioType;}

	void SetSoundCardSelection(qint32 SoundInIndex, qint32 SoundOutIndex,bool StereoOut)
				{ m_pSoundCardOut->Stop(); m_SoundInIndex = SoundInIndex;
					m_SoundOutIndex = SoundOutIndex;  m_StereoOut = StereoOut;}

	int GetRateError(){return m_pSoundCardOut->GetRateError();}
	void SetVolume(qint32 vol){ m_pSoundCardOut->SetVolume(vol); }

	void SetChannelMode(qint32 channelmode);

	void SetSdrRfGain(qint32 gain);
	qint32 GetSdrRfGain(){return m_RfGain;}

	void SetSdrBandwidthIndex(qint32 bwindex);
	double GetSdrSampleRate(){return m_SampleRate;}
	quint32 GetSdrMaxBandwidth(){return m_SampleRate;}

	void SetFftSize(qint32 size);
	quint32 GetSdrFftSize(){return m_FftSize;}

	void SetFftAve(qint32 ave);
	quint32 GetSdrFftAve(){return m_FftAve;}

	qint32 GetMaxBWFromIndex(qint32 index);
	double GetSampleRateFromIndex(qint32 index);

	void SetMaxDisplayRate(int updatespersec){m_MaxDisplayRate = updatespersec;
							m_DisplaySkipValue = m_SampleRate/(m_FftSize*m_MaxDisplayRate);
							m_DisplaySkipCounter = 0; }

	void SetDemod(int Mode, tDemodInfo CurrentDemodInfo);
	void SetDemodFreq(qint64 Freq){m_Demodulator.SetDemodFreq((TYPEREAL)Freq);}

	void SetupNoiseProc(tNoiseProcdInfo* pNoiseProcSettings);

	double GetSMeterPeak(){return m_Demodulator.GetSMeterPeak() + m_GainCalibrationOffset - m_RfGain;}
	double GetSMeterAve(){return m_Demodulator.GetSMeterAve() + m_GainCalibrationOffset - m_RfGain;}


signals:
	void NewStatus(int status);		//emitted when sdr status changes
	void NewInfoData();				//emitted when sdr information is received after GetSdrInfo()
	void NewFftData();				//emitted when new FFT data is available
	void FreqChange(int freq);	//emitted if requested frequency has been clamped by radio

private:
	void SendAck(quint8 chan);
	void Start6620Download();
	void NcoSpurCalibrate(double* pData, qint32 length);


	bool m_Running;
	bool m_ScreenUpateFinished;
	bool m_StereoOut;
	qint32 m_BandwidthIndex;
	qint32 m_DisplaySkipCounter;
	qint32 m_DisplaySkipValue;
	qint32 m_FftSize;
	qint32 m_FftAve;
	qint32 m_FftBufPos;
	qint32 m_KeepAliveCounter;
	qint32 m_MaxBandwidth;
	qint32 m_MaxDisplayRate;
	qint32 m_RfGain;
	qint32 m_SoundInIndex;
	qint32 m_SoundOutIndex;
	qint32 m_ChannelMode;
	quint8 m_Channel;
	quint64 m_CurrentFrequency;
	quint64 m_BaseFrequencyRangeMin;
	quint64 m_BaseFrequencyRangeMax;
	quint64 m_OptionFrequencyRangeMin;
	quint64 m_OptionFrequencyRangeMax;
	double m_SampleRate;
	double m_GainCalibrationOffset;
	double m_DataBuf[MAX_FFT_SIZE*2];

	CAscpMsg m_TxMsg;
	Cad6620 m_AD6620;
	eRadioType m_RadioType;
	eStatus m_Status;

	bool m_NcoSpurCalActive;	//NCO spur reduction variables
	qint32 m_NcoSpurCalCount;
	double m_NCOSpurOffsetI;
	double m_NCOSpurOffsetQ;

	CFft m_Fft;
	CDemodulator m_Demodulator;
	CNoiseProc m_NoiseProc;
	CSoundOut* m_pSoundCardOut;

CIir m_Iir;

};



#endif // SDRINTERFACE_H
