//////////////////////////////////////////////////////////////////////
// testbench.h: interface of the CTestBench class.
//
//  This class creates a test bench dialog that generates complex
// signals and displays complex data and spectrum for testing and debug
//
// History:
//	2010-12-18  Initial creation MSW
//	2011-03-27  Initial release
//////////////////////////////////////////////////////////////////////
#ifndef TESTBENCH_H
#define TESTBENCH_H

#include <QDialog>
#include <QtGui>
#include <QFrame>
#include <QImage>

#include "dsp/datatypes.h"
#include "dsp/fft.h"


//////////////////////////////////////////////////////////////////////
//  global defines
//////////////////////////////////////////////////////////////////////

// Profile Defines.  Used to select various test points
//within the program
#define PROFILE_OFF 0
#define PROFILE_1 1
#define PROFILE_2 2
#define PROFILE_3 3
#define PROFILE_4 4
#define PROFILE_5 5
#define PROFILE_6 6
#define PROFILE_7 7

#define NUM_PROFILES 8


#define TB_VERT_DIVS 18	//specify grid screen divisions
#define TB_HORZ_DIVS 10
#define TB_TIMEVERT_DIVS 10	//specify time display grid screen divisions
#define TEST_FFTSIZE 2048

#define TB_MAX_SCREENSIZE 2048


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
}tBtoL2;


namespace Ui {
    class CTestBench;
}

class CTestBench : public QDialog
{
    Q_OBJECT

public:
    explicit CTestBench(QWidget *parent = 0);
    ~CTestBench();
	void Init();	//called to initialize controls after setting all class variables

	void CreateGeneratorSamples(int length, TYPECPX* pBuf, double samplerate);
	void CreateGeneratorSamples(int length, TYPEREAL* pBuf, double samplerate);
	// overloaded data display routines
	void DisplayData(int n, TYPEREAL* pBuf, double samplerate, int profile);
	void DisplayData(int n, TYPECPX* pBuf, double samplerate, int profile);
	void DisplayData(int n, TYPEMONO16* pBuf, double samplerate, int profile);
	void DisplayData(int n, TYPESTEREO16* pBuf, double samplerate, int profile);

	void SendDebugTxt(QString Str){ if(m_Active) emit SendTxt(Str);}

	//Exposed Dialog Class variables for persistant saving/restoring by parent
	bool m_TimeDisplay;
	bool m_GenOn;
	bool m_PeakOn;
	bool m_NewDataIsCpx;
	bool m_CurrentDataIsCpx;
	int m_Profile;
	int m_TrigIndex;
	int m_DisplayRate;
	int m_HorzSpan;
	int m_VertRange;
	int m_TrigLevel;
	double m_PulseWidth;
	double m_PulsePeriod;
	double m_SignalPower;
	double m_NoisePower;
	double m_SweepStartFrequency;
	double m_SweepStopFrequency;
	double m_SweepRate;

public slots:
	void Reset();		//called by GUI Reset button
	void DrawFftPlot();	//called to draw new fft data onto screen plot
	void DrawTimePlot();	//called to draw new Time data onto screen plot
	void GotTxt(QString);
	void OnTimer();

	void OnGenOn(bool On);
	void OnTimeDisplay(bool timemode);
	void OnEnablePeak(bool enablepeak);
	void OnSweepStart(int start);
	void OnSweepStop(int stop);
	void OnSweepRate(int rate);
	void OnDisplayRate(int rate);
	void OnVertRange(int range);
	void OnHorzSpan(int span);
	void OnTrigLevel(int level);
	void OnTriggerMode(int trigindex);
	void OnProfile(int profindex);
	void OnPulseWidth(int pwidth);
	void OnPulsePeriod(int pperiod);
	void OnSignalPwr(int pwr);
	void OnNoisePwr(int pwr);

signals:
	void ResetSignal();		//internal signals from worker thread called functions
	void NewFftData();
	void NewTimeData();
	void SendTxt(QString);

protected:
		//re-implemented widget event handlers
	void resizeEvent(QResizeEvent* event);
	void paintEvent(QPaintEvent *event);
	void closeEvent(QCloseEvent *event);
	void showEvent(QShowEvent *event);

private:
    Ui::CTestBench *ui;
	void DrawFreqOverlay();
	void DrawTimeOverlay();
	void MakeFrequencyStrs();
	void ChkForTrigger(qint32 sample);
	quint64 rdtsctime();
	QPixmap m_2DPixmap;
	QPixmap m_OverlayPixmap;
	QSize m_Size;
	QRect m_Rect;
	QTimer *m_pTimer;
	bool m_Active;
	qint32 m_Span;
	qint32 m_MaxdB;
	qint32 m_MindB;
	qint32 m_dBStepSize;
	qint32 m_FreqUnits;
	qint64 m_CenterFreq;
	double m_GenSampleRate;
	double m_DisplaySampleRate;
	QString m_Str;
	QString m_HDivText[TB_HORZ_DIVS+1];
	TYPECPX m_FftInBuf[TEST_FFTSIZE];
	qint32 m_FftPkBuf[TB_MAX_SCREENSIZE];
	qint32 m_TimeBuf1[TB_MAX_SCREENSIZE];
	qint32 m_TimeBuf2[TB_MAX_SCREENSIZE];
	qint32 m_TimeScrnBuf1[TB_MAX_SCREENSIZE];
	qint32 m_TimeScrnBuf2[TB_MAX_SCREENSIZE];
	qint32 m_PreviousSample;

	int m_FftBufPos;
	qint32 m_DisplaySkipValue;
	qint32 m_DisplaySkipCounter;
	int m_TimeScrnPos;
	int m_TimeInPos;
	int m_TrigBufPos;
	int m_TrigState;
	int m_TrigCounter;
	int m_PostScrnCaptureLength;
	double m_TimeScrnPixel;

	double m_SweepFrequency;
	double m_SweepFreqNorm;
	double m_SweepAcc;
	double m_SweepRateInc;
	double m_SignalAmplitude;
	double m_NoiseAmplitude;
	double m_PulseTimer;

	CFft m_Fft;
	QFile m_File;

};

extern CTestBench* g_pTestBench;

#endif // TESTBENCH_H
