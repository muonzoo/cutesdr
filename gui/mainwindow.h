//////////////////////////////////////////////////////////////////////
// mainwindow.h: interface for the MainWindow GUI class.
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
/////////////////////////////////////////////////////////////////////
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "interface/sdrinterface.h"
#include <QHostAddress>
#include "gui/demodsetupdlg.h"



namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

	void SetupDemod(int index);
	void SetupNoiseProc();
	tDemodInfo m_DemodSettings[NUM_DEMODS];	//not all fields are saved in Settings

private slots:
	void closeEvent(QCloseEvent *event);
	void AlwaysOnTop();
	void OnExit();
	void OnAbout();
	void OnDisplayDlg();
	void OnSoundCardDlg();
	void OnSdrDlg();
	void OnNetworkDlg();
	void OnDemodDlg();
	void OnNoiseProcDlg();
	void OnVolumeSlider(int value);

	void OnRun();
	void OnSpanChanged(int spanKhz);
	void OnMaxdBChanged(int maxdb);
	void OnVertScaleChanged(int index);

	void OnTimer();
	void OnStatus(int status);
	void OnNewInfoData();
	void OnNewFftData();
	void OnNewScreenDemodFreq(qint64 freq);
	void OnNewScreenCenterFreq(qint64 freq);
	void OnNewCenterFrequency(qint64 freq);	//called when center frequency has changed
	void OnNewDemodFrequency(qint64 freq);	//called when demod frequency has changed
	void OnNewLowCutFreq(int freq);
	void OnNewHighCutFreq(int freq);

protected:
	void mousePressEvent(QMouseEvent *);

private:
	void readSettings();
	void writeSettings();
	void UpdateInfoBox();
	void InitDemodSettings();

	/////////////////////////
	//Persistant Settings Variables saved
	/////////////////////////
	bool m_StereoOut;
	bool m_UseTestBench;
	bool m_AlwaysOnTop;
	bool m_AgcOn;
	qint64 m_CenterFrequency;
	qint64 m_DemodFrequency;
	quint32 m_SpanFrequency;
	QHostAddress m_IPAdr;
	quint32 m_Port;
	qint32 m_RadioType;
	qint32 m_BandwidthIndex;
	qint32 m_SoundInIndex;
	qint32 m_SoundOutIndex;
	qint32 m_ClickResolution;
	qint32 m_MaxDisplayRate;
	qint32 m_VertScaleIndex;
	qint32 m_dBStepSize;
	qint32 m_MaxdB;
	qint32 m_RfGain;
	qint32 m_LastSpanKhz;
	qint32 m_FftAve;
	qint32 m_FftSize;
	qint32 m_Volume;
	qint32 m_Percent2DScreen;
	qint32 m_DemodMode;
	double m_NCOSpurOffsetI;	//NCO spur reduction variables
	double m_NCOSpurOffsetQ;
	tNoiseProcdInfo m_NoiseProcSettings;

	QRect m_TestBenchRect;
	QString m_Str;
	QString m_Str2;
	QString m_ActiveDevice;
	CSdrInterface::eStatus m_Status;
	CSdrInterface::eStatus m_LastStatus;
	QTimer *m_pTimer;
	CSdrInterface* m_pSdrInterface;
	CDemodSetupDlg* m_pDemodSetupDlg;
	qint32 m_KeepAliveTimer;
	Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
