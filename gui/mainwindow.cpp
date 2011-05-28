/////////////////////////////////////////////////////////////////////
// mainwindow.cpp: implementation of the mainwindow class.
//
//	This class implements the top level GUI control and is the primary
// object instantiated by main().  All other classes and threads are
// spawned from this module.
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
//	2011-04-16  Added Frequency range logic
//	2011-05-26  Added support for In Use Status allowed wider sideband filters
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

#include "gui/mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include "gui/freqctrl.h"
#include "gui/editnetdlg.h"
#include "gui/sdrsetupdlg.h"
#include "gui/noiseprocdlg.h"
#include "gui/sounddlg.h"
#include "gui/displaydlg.h"
#include "gui/aboutdlg.h"
#include "interface/perform.h"


#define DISCOVER_CLIENT_PORT 48322	/* PC client Rx port, SDR Server Tx Port */
QUdpSocket g_UdpDiscoverSocket;	//global hack to get around bug in UDP sockets


/*---------------------------------------------------------------------------*/
/*--------------------> L O C A L   D E F I N E S <--------------------------*/
/*---------------------------------------------------------------------------*/
#define PROGRAM_TITLE_VERSION "CuteSdr 1.02 "

#define MAX_FFTDB 60
#define MIN_FFTDB -170

/////////////////////////////////////////////////////////////////////
// Constructor/Destructor
/////////////////////////////////////////////////////////////////////
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	setWindowTitle(PROGRAM_TITLE_VERSION);

	//create SDR interface class
	m_pSdrInterface = new CSdrInterface;
	//give GUI plotter access to the sdr interface object
	ui->framePlot->SetSdrInterface(m_pSdrInterface);

	//create the global Testbench object
	if(!g_pTestBench)
		g_pTestBench = new CTestBench(this);

	readSettings();		//read persistent settings

	ui->actionAlwaysOnTop->setChecked(m_AlwaysOnTop);
	AlwaysOnTop();

	//create Demod setup menu for non-modal use(can leave up and still access rest of program)
	m_pDemodSetupDlg = new CDemodSetupDlg(this);

	m_pTimer = new QTimer(this);

	//connect a bunch of signals to the GUI objects
	connect(m_pTimer, SIGNAL(timeout()), this, SLOT(OnTimer()));

	connect(ui->frameFreqCtrl, SIGNAL(NewFrequency(qint64)), this, SLOT(OnNewCenterFrequency(qint64)));
	connect(ui->frameDemodFreqCtrl, SIGNAL(NewFrequency(qint64)), this, SLOT(OnNewDemodFrequency(qint64)));

	connect(m_pSdrInterface, SIGNAL(NewStatus(int)), this,  SLOT( OnStatus(int) ) );
	connect(m_pSdrInterface, SIGNAL(NewInfoData()), this,  SLOT( OnNewInfoData() ) );
	connect(m_pSdrInterface, SIGNAL(NewFftData()), this,  SLOT( OnNewFftData() ) );

	connect(ui->actionExit, SIGNAL(triggered()), this, SLOT(OnExit()));
	connect(ui->actionNetwork, SIGNAL(triggered()), this, SLOT(OnNetworkDlg()));
	connect(ui->actionSoundCard, SIGNAL(triggered()), this, SLOT(OnSoundCardDlg()));
	connect(ui->actionSDR, SIGNAL(triggered()), this, SLOT(OnSdrDlg()));
	connect(ui->actionDisplay, SIGNAL(triggered()), this, SLOT(OnDisplayDlg()));
	connect(ui->actionAlwaysOnTop, SIGNAL(triggered()), this, SLOT(AlwaysOnTop()));
	connect(ui->actionDemod_Setup, SIGNAL(triggered()), this, SLOT(OnDemodDlg()));
	connect(ui->actionNoise_Processing, SIGNAL(triggered()), this, SLOT(OnNoiseProcDlg()));

	connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(OnAbout()));

	connect(ui->framePlot, SIGNAL(NewDemodFreq(qint64)), this,  SLOT( OnNewScreenDemodFreq(qint64) ) );
	connect(ui->framePlot, SIGNAL(NewCenterFreq(qint64)), this,  SLOT( OnNewScreenCenterFreq(qint64) ) );
	connect(ui->framePlot, SIGNAL(NewLowCutFreq(int)), this,  SLOT( OnNewLowCutFreq(int) ) );
	connect(ui->framePlot, SIGNAL(NewHighCutFreq(int)), this,  SLOT( OnNewHighCutFreq(int) ) );

	m_pTimer->start(200);		//start up status timer

	m_pSdrInterface->SetRadioType(m_RadioType);
		quint32 maxspan = m_pSdrInterface->GetMaxBWFromIndex(m_BandwidthIndex);

	ui->framePlot->SetPercent2DScreen(m_Percent2DScreen);

	//initialize controls and limits
	ui->SpanspinBox->setMaximum(maxspan/1000);
	if(m_SpanFrequency>maxspan)
		m_SpanFrequency = maxspan;
	ui->SpanspinBox->setValue(m_SpanFrequency/1000);
	m_LastSpanKhz = m_SpanFrequency/1000;

	//tmp save demod freq since gets set to center freq by center freq control inititalization
	qint64 tmpdemod = m_DemodFrequency;

	ui->frameFreqCtrl->Setup(9, 100U, 500000000U, 1, UNITS_KHZ );
	ui->frameFreqCtrl->SetBkColor(Qt::darkBlue);
	ui->frameFreqCtrl->SetDigitColor(Qt::cyan);
	ui->frameFreqCtrl->SetUnitsColor(Qt::lightGray);
	ui->frameFreqCtrl->SetHighlightColor(Qt::darkGray);
	ui->frameFreqCtrl->SetFrequency(m_CenterFrequency);

	m_DemodFrequency = tmpdemod;
	ui->frameDemodFreqCtrl->Setup(9, 100U, 500000000U, 1, UNITS_KHZ );
	ui->frameDemodFreqCtrl->SetBkColor(Qt::darkBlue);
	ui->frameDemodFreqCtrl->SetDigitColor(Qt::white);
	ui->frameDemodFreqCtrl->SetUnitsColor(Qt::lightGray);
	ui->frameDemodFreqCtrl->SetHighlightColor(Qt::darkGray);
	//limit demod frequency to Center Frequency +/-span frequency
	ui->frameDemodFreqCtrl->Setup(9, m_CenterFrequency-m_SpanFrequency/2,
								  m_CenterFrequency+m_SpanFrequency/2,
								  1,
								  UNITS_KHZ );
	ui->frameDemodFreqCtrl->SetFrequency(m_DemodFrequency);

	ui->framePlot->SetSpanFreq( m_SpanFrequency );
	ui->framePlot->SetCenterFreq( m_CenterFrequency );
	ui->framePlot->SetClickResolution(m_ClickResolution);

	ui->horizontalSliderVol->setValue(m_Volume);
	m_pSdrInterface->SetVolume(m_Volume);

	ui->ScalecomboBox->addItem("10 dB/Div", 10);
	ui->ScalecomboBox->addItem("5 dB/Div", 5);
	ui->ScalecomboBox->addItem("3 dB/Div", 3);
	ui->ScalecomboBox->addItem("1 dB/Div", 1);
	m_dBStepSize = (int)ui->ScalecomboBox->itemData(m_VertScaleIndex).toInt();
	ui->ScalecomboBox->setCurrentIndex(m_VertScaleIndex);
	ui->framePlot->SetdBStepSize(m_dBStepSize);

	ui->MaxdBspinBox->setValue(m_MaxdB);
	ui->MaxdBspinBox->setSingleStep(m_dBStepSize);
	ui->MaxdBspinBox->setMinimum(MIN_FFTDB+VERT_DIVS*m_dBStepSize);
	ui->MaxdBspinBox->setMaximum(MAX_FFTDB);
	ui->framePlot->SetMaxdB(m_MaxdB);


	m_pSdrInterface->SetFftSize( m_FftSize);
	m_pSdrInterface->SetFftAve( m_FftAve);
	m_pSdrInterface->SetMaxDisplayRate(m_MaxDisplayRate);
	m_pSdrInterface->SetSdrBandwidthIndex(m_BandwidthIndex);
	m_pSdrInterface->SetSdrRfGain( m_RfGain );
	m_pSdrInterface->ManageNCOSpurOffsets(CSdrInterface::NCOSPUR_CMD_SET,
										  &m_NCOSpurOffsetI,
										  &m_NCOSpurOffsetQ);

	m_pSdrInterface->SetSoundCardSelection(m_SoundInIndex, m_SoundOutIndex, m_StereoOut);

	InitDemodSettings();
	ui->framePlot->SetDemodCenterFreq( m_DemodFrequency );
	SetupDemod(m_DemodMode);

	SetupNoiseProc();

	UpdateInfoBox();

	m_ActiveDevice = "";
	m_pSdrInterface->SetupNetwork(m_IPAdr,m_Port);
	m_Status = CSdrInterface::NOT_CONNECTED;
	m_LastStatus = m_Status;
	m_pSdrInterface->StartIO();

	m_KeepAliveTimer = 0;


	if(m_UseTestBench)
	{
		//make sure top of dialog is visable(0,0 doesn't include menu bar.Qt bug?)
		if(m_TestBenchRect.top()<30)
			m_TestBenchRect.setTop(30);
		g_pTestBench->setGeometry(m_TestBenchRect);
		g_pTestBench->show();
		g_pTestBench->Init();
	}

	//QT bug, can only bind once on some Windows machines??
	g_UdpDiscoverSocket.bind(DISCOVER_CLIENT_PORT);
}

MainWindow::~MainWindow()
{
	if(g_pTestBench)
		delete g_pTestBench;
	if(m_pSdrInterface)
	{
		m_pSdrInterface->StopIO();
		delete m_pSdrInterface;
	}
	if(m_pDemodSetupDlg)
		delete m_pDemodSetupDlg;
	g_UdpDiscoverSocket.close();
	delete ui;
}

/////////////////////////////////////////////////////////////////////
// Called when program is closed to save persistant data
/////////////////////////////////////////////////////////////////////
void MainWindow::closeEvent(QCloseEvent *)
 {
	writeSettings();
	if(m_pSdrInterface)
		m_pSdrInterface->StopIO();
 }

/////////////////////////////////////////////////////////////////////
// Called to set "Always on Top" Main Window state
/////////////////////////////////////////////////////////////////////
void MainWindow::AlwaysOnTop()
{
	m_AlwaysOnTop = ui->actionAlwaysOnTop->isChecked();
	Qt::WindowFlags flags = this->windowFlags();
	if (m_AlwaysOnTop)
	{
		this->setWindowFlags(flags | Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint);
		this->show();
	}
	else
	{
		this->setWindowFlags( (flags | Qt::CustomizeWindowHint) & ~Qt::WindowStaysOnTopHint );
		this->show();
	}
}

/////////////////////////////////////////////////////////////////////
// Program persistant data save/recall methods
/////////////////////////////////////////////////////////////////////
void MainWindow::writeSettings()
{
	QSettings settings( QSettings::UserScope,"MoeTronix", "CuteSdr");
	settings.beginGroup("MainWindow");

	settings.setValue("geometry", saveGeometry());
	settings.setValue("minstate",isMinimized());

	if(g_pTestBench->isVisible())
	{
		m_TestBenchRect = g_pTestBench->geometry();
		settings.setValue("TestBenchRect",m_TestBenchRect);
	}

	settings.endGroup();

	settings.beginGroup("Common");

	settings.setValue("RadioType", m_RadioType);
	settings.setValue("CenterFrequency",(int)m_CenterFrequency);
	settings.setValue("SpanFrequency",(int)m_SpanFrequency);
	settings.setValue("IPAdr",m_IPAdr.toIPv4Address());
	settings.setValue("Port",m_Port);
	settings.setValue("RfGain",m_RfGain);
	settings.setValue("BandwidthIndex", m_BandwidthIndex );
	settings.setValue("SoundInIndex",m_SoundInIndex);
	settings.setValue("SoundOutIndex",m_SoundOutIndex);
	settings.setValue("StereoOut",m_StereoOut);
	settings.setValue("VertScaleIndex",m_VertScaleIndex);
	settings.setValue("MaxdB",m_MaxdB);
	settings.setValue("FftSize",m_FftSize);
	settings.setValue("FftAve",m_FftAve);
	settings.setValue("MaxDisplayRate",m_MaxDisplayRate);
	settings.setValue("ClickResolution",m_ClickResolution);
	settings.setValue("UseTestBench",m_UseTestBench);
	settings.setValue("AlwaysOnTop",m_AlwaysOnTop);
	settings.setValue("Volume",m_Volume);
	settings.setValue("Percent2DScreen",m_Percent2DScreen);

	//Get NCO spur offsets and save
	m_pSdrInterface->ManageNCOSpurOffsets(CSdrInterface::NCOSPUR_CMD_READ,
										  &m_NCOSpurOffsetI,
										  &m_NCOSpurOffsetQ);
	settings.setValue("NCOSpurOffsetI",m_NCOSpurOffsetI);
	settings.setValue("NCOSpurOffsetQ",m_NCOSpurOffsetQ);

	settings.setValue("DemodFrequency",(int)m_DemodFrequency);
	settings.setValue("DemodMode",m_DemodMode);

	settings.setValue("NBOn",m_NoiseProcSettings.NBOn);
	settings.setValue("NBThreshold",m_NoiseProcSettings.NBThreshold);
	settings.setValue("NBWidth",m_NoiseProcSettings.NBWidth);

	settings.endGroup();

	settings.beginGroup("Testbench");

	settings.setValue("SweepStartFrequency",g_pTestBench->m_SweepStartFrequency);
	settings.setValue("SweepStopFrequency",g_pTestBench->m_SweepStopFrequency);
	settings.setValue("SweepRate",g_pTestBench->m_SweepRate);
	settings.setValue("DisplayRate",g_pTestBench->m_DisplayRate);
	settings.setValue("VertRange",g_pTestBench->m_VertRange);
	settings.setValue("TrigIndex",g_pTestBench->m_TrigIndex);
	settings.setValue("TimeDisplay",g_pTestBench->m_TimeDisplay);
	settings.setValue("HorzSpan",g_pTestBench->m_HorzSpan);
	settings.setValue("TrigLevel",g_pTestBench->m_TrigLevel);
	settings.setValue("Profile",g_pTestBench->m_Profile);
	settings.setValue("GenOn",g_pTestBench->m_GenOn);
	settings.setValue("PeakOn",g_pTestBench->m_PeakOn);
	settings.setValue("PulseWidth",g_pTestBench->m_PulseWidth);
	settings.setValue("PulsePeriod",g_pTestBench->m_PulsePeriod);
	settings.setValue("SignalPower",g_pTestBench->m_SignalPower);
	settings.setValue("NoisePower",g_pTestBench->m_NoisePower);

	settings.endGroup();

	settings.beginWriteArray("Demod");
	//save demod settings
	for (int i = 0; i < NUM_DEMODS; i++)
	{
		settings.setArrayIndex(i);
		settings.setValue("HiCut", m_DemodSettings[i].HiCut);
		settings.setValue("LowCut", m_DemodSettings[i].LowCut);
		settings.setValue("FilterClickResolution", m_DemodSettings[i].FilterClickResolution);
		settings.setValue("Offset", m_DemodSettings[i].Offset);
		settings.setValue("SquelchValue", m_DemodSettings[i].SquelchValue);
		settings.setValue("AgcSlope", m_DemodSettings[i].AgcSlope);
		settings.setValue("AgcThresh", m_DemodSettings[i].AgcThresh);
		settings.setValue("AgcManualGain", m_DemodSettings[i].AgcManualGain);
		settings.setValue("AgcDecay", m_DemodSettings[i].AgcDecay);
		settings.setValue("AgcOn", m_DemodSettings[i].AgcOn);
		settings.setValue("AgcHangOn", m_DemodSettings[i].AgcHangOn);
	}
	settings.endArray();
}

void MainWindow::readSettings()
{
	QSettings settings(QSettings::UserScope,"MoeTronix", "CuteSdr");
	settings.beginGroup("MainWindow");

	restoreGeometry(settings.value("geometry").toByteArray());
	bool ismin = settings.value("minstate", false).toBool();
	m_TestBenchRect = settings.value("TestBenchRect", QRect(0,0,500,200)).toRect();

	settings.endGroup();

	settings.beginGroup("Common");

	m_CenterFrequency = (qint64)settings.value("CenterFrequency", 15000000).toUInt();
	m_SpanFrequency = settings.value("SpanFrequency", 100000).toUInt();
	m_IPAdr.setAddress(settings.value("IPAdr", 0xC0A80164).toInt() );
	m_Port = settings.value("Port", 50000).toUInt();
	m_RfGain = settings.value("RfGain", 0).toInt();
	m_BandwidthIndex = settings.value("BandwidthIndex", 0).toInt();
	m_SoundInIndex = settings.value("SoundInIndex", 0).toInt();
	m_SoundOutIndex = settings.value("SoundOutIndex", 0).toInt();
	m_StereoOut = settings.value("StereoOut", false).toBool();
	m_VertScaleIndex = settings.value("VertScaleIndex", 0).toInt();
	m_MaxdB = settings.value("MaxdB", 0).toInt();
	m_FftAve = settings.value("FftAve", 0).toInt();
	m_FftSize = settings.value("FftSize", 4096).toInt();
	m_MaxDisplayRate = settings.value("MaxDisplayRate", 10).toInt();
	m_RadioType = settings.value("RadioType", 0).toInt();
	m_ClickResolution = settings.value("ClickResolution",100).toInt();
	m_Volume = settings.value("Volume",100).toInt();
	m_Percent2DScreen = settings.value("Percent2DScreen",50).toInt();

	m_NCOSpurOffsetI = settings.value("NCOSpurOffsetI",0.0).toDouble();
	m_NCOSpurOffsetQ = settings.value("NCOSpurOffsetQ",0.0).toDouble();

	m_UseTestBench = settings.value("UseTestBench", false).toBool();
	m_AlwaysOnTop = settings.value("AlwaysOnTop", false).toBool();

	m_NoiseProcSettings.NBOn = settings.value("NBOn", false).toBool();
	m_NoiseProcSettings.NBThreshold = settings.value("NBThreshold",0).toInt();
	m_NoiseProcSettings.NBWidth = settings.value("NBWidth",50).toInt();


	m_DemodMode = settings.value("DemodMode", DEMOD_AM).toInt();
	m_DemodFrequency = (qint64)settings.value("DemodFrequency", 15000000).toUInt();

	settings.endGroup();

	settings.beginGroup("Testbench");

	g_pTestBench->m_SweepStartFrequency = settings.value("SweepStartFrequency",0.0).toDouble();
	g_pTestBench->m_SweepStopFrequency = settings.value("SweepStopFrequency",1.0).toDouble();
	g_pTestBench->m_SweepRate = settings.value("SweepRate",0.0).toDouble();
	g_pTestBench->m_DisplayRate = settings.value("DisplayRate",10).toInt();
	g_pTestBench->m_VertRange = settings.value("VertRange",10000).toInt();
	g_pTestBench->m_TrigIndex = settings.value("TrigIndex",0).toInt();
	g_pTestBench->m_TrigLevel = settings.value("TrigLevel",100).toInt();
	g_pTestBench->m_HorzSpan = settings.value("HorzSpan",100).toInt();
	g_pTestBench->m_Profile = settings.value("Profile",0).toInt();
	g_pTestBench->m_TimeDisplay = settings.value("TimeDisplay",false).toBool();
	g_pTestBench->m_GenOn = settings.value("GenOn",false).toBool();
	g_pTestBench->m_PeakOn = settings.value("PeakOn",false).toBool();
	g_pTestBench->m_PulseWidth = settings.value("PulseWidth",0.0).toDouble();
	g_pTestBench->m_PulsePeriod = settings.value("PulsePeriod",0.0).toDouble();
	g_pTestBench->m_SignalPower = settings.value("SignalPower",0.0).toDouble();
	g_pTestBench->m_NoisePower = settings.value("NoisePower",0.0).toDouble();

	settings.endGroup();

	settings.beginReadArray("Demod");
	//get demod settings
	for (int i = 0; i < NUM_DEMODS; i++)
	{
		settings.setArrayIndex(i);
		m_DemodSettings[i].HiCut = settings.value("HiCut", 5000).toInt();
		m_DemodSettings[i].LowCut = settings.value("LowCut", -5000).toInt();
		m_DemodSettings[i].FilterClickResolution = settings.value("FilterClickResolution", 100).toInt();
		m_DemodSettings[i].Offset = settings.value("Offset", 0).toInt();
		m_DemodSettings[i].SquelchValue = settings.value("SquelchValue", 0).toInt();
		m_DemodSettings[i].AgcSlope = settings.value("AgcSlope", 0).toInt();
		m_DemodSettings[i].AgcThresh = settings.value("AgcThresh", -100).toInt();
		m_DemodSettings[i].AgcManualGain = settings.value("AgcManualGain", 30).toInt();
		m_DemodSettings[i].AgcDecay = settings.value("AgcDecay", 200).toInt();
		m_DemodSettings[i].AgcOn = settings.value("AgcOn",true).toBool();
		m_DemodSettings[i].AgcHangOn = settings.value("AgcHangOn",false).toBool();
	}
	settings.endArray();

	if(ismin)
		showMinimized();
}

/////////////////////////////////////////////////////////////////////
// Status Timer event handler
/////////////////////////////////////////////////////////////////////
void MainWindow::OnTimer()
{

	OnStatus(m_Status);
	if(++m_KeepAliveTimer>5)
	{
		m_KeepAliveTimer = 0;
		if( (CSdrInterface::RUNNING == m_Status) || ( CSdrInterface::CONNECTED == m_Status) )
			m_pSdrInterface->KeepAlive();
	}
	ui->frameMeter->SetdBmLevel( m_pSdrInterface->GetSMeterAve() );
}

/////////////////////////////////////////////////////////////////////
// Mouse Right button event handler brings up Demod dialog
/////////////////////////////////////////////////////////////////////
void MainWindow::mousePressEvent(QMouseEvent *event)
{
	if(Qt::RightButton==event->button() )
	{
		OnDemodDlg();
	}
}


/////////////////////////////////////////////////////////////////////
// About Dialog Menu Bar action item handler.
/////////////////////////////////////////////////////////////////////
void MainWindow::OnAbout()
{
CAboutDlg dlg(this,PROGRAM_TITLE_VERSION);
	dlg.exec();
}

/////////////////////////////////////////////////////////////////////
// Menu Bar action item handler.
//Exit menu
/////////////////////////////////////////////////////////////////////
void MainWindow::OnExit()
{
	qApp->exit(0);
}

/////////////////////////////////////////////////////////////////////
// Menu Bar action item handler.
//Display Setup Menu
/////////////////////////////////////////////////////////////////////
void MainWindow::OnDisplayDlg()
{
CDisplayDlg dlg(this);
	dlg.m_FftSize = m_FftSize;
	dlg.m_FftAve = m_FftAve;
	dlg.m_ClickResolution = m_ClickResolution;
	dlg.m_MaxDisplayRate = m_MaxDisplayRate;
	dlg.m_UseTestBench = m_UseTestBench;
	dlg.m_Percent2DScreen = m_Percent2DScreen;
	dlg.InitDlg();
	if(QDialog::Accepted == dlg.exec() )
	{
		if(dlg.m_NeedToStop)
		{
			if(CSdrInterface::RUNNING == m_Status)
			{
				m_pSdrInterface->StopSdr();
				ui->framePlot->SetRunningState(false);
			}
		}
		if(m_Percent2DScreen != dlg.m_Percent2DScreen)
		{	//if 2D screen size changed then update it
			m_Percent2DScreen = dlg.m_Percent2DScreen;
			ui->framePlot->SetPercent2DScreen(m_Percent2DScreen);
		}
		m_FftSize = dlg.m_FftSize;
		m_FftAve = dlg.m_FftAve;
		m_ClickResolution = dlg.m_ClickResolution;
		m_MaxDisplayRate = dlg.m_MaxDisplayRate;
		m_UseTestBench = dlg.m_UseTestBench;
		m_pSdrInterface->SetFftAve( m_FftAve);
		m_pSdrInterface->SetFftSize( m_FftSize);
		m_pSdrInterface->SetMaxDisplayRate(m_MaxDisplayRate);
		ui->framePlot->SetClickResolution(m_ClickResolution);
		if(m_UseTestBench)
		{	//make TestBench visable if not already
			if(!g_pTestBench->isVisible())
			{
				//make sure top of dialog is visable(0,0 doesn't include menu bar.Qt bug?)
				if(m_TestBenchRect.top()<30)
					m_TestBenchRect.setTop(30);
				g_pTestBench->setGeometry(m_TestBenchRect);
				g_pTestBench->show();
				g_pTestBench->Init();
			}
			g_pTestBench->activateWindow();
		}
		else
		{	//hide testbench
			if(g_pTestBench->isVisible())
				g_pTestBench->hide();
		}
	}
}

/////////////////////////////////////////////////////////////////////
// Menu Bar action item handler.
//Sound Card Setup Menu
/////////////////////////////////////////////////////////////////////
void MainWindow::OnSoundCardDlg()
{
CSoundDlg dlg(this);
	dlg.SetInputIndex(m_SoundInIndex);
	dlg.SetOutputIndex(m_SoundOutIndex);
	dlg.SetStereo(m_StereoOut);
	if(QDialog::Accepted == dlg.exec() )
	{
		if(CSdrInterface::RUNNING == m_Status)
		{
			m_pSdrInterface->StopSdr();
			ui->framePlot->SetRunningState(false);
		}
		m_StereoOut = dlg.GetStereo();
		m_SoundInIndex = dlg.GetInputIndex();
		m_SoundOutIndex = dlg.GetOutputIndex();
		m_pSdrInterface->SetSoundCardSelection(m_SoundInIndex, m_SoundOutIndex, m_StereoOut);
	}
}

/////////////////////////////////////////////////////////////////////
// Menu Bar action item handler.
//SDR Setup Menu
/////////////////////////////////////////////////////////////////////
void MainWindow::OnSdrDlg()
{
CSdrSetupDlg dlg(this,m_pSdrInterface);
	dlg.m_BandwidthIndex = m_BandwidthIndex;
	dlg.InitDlg();
	if( dlg.exec() )
	{
		if( m_BandwidthIndex != dlg.m_BandwidthIndex )
		{
			m_BandwidthIndex = dlg.m_BandwidthIndex;
			if(CSdrInterface::RUNNING == m_Status)
			{
				m_pSdrInterface->StopSdr();
				ui->framePlot->SetRunningState(false);
			}
		}
		SetupNoiseProc();
		m_RfGain = dlg.m_RfGain;
		m_pSdrInterface->SetSdrRfGain( dlg.m_RfGain);
		m_pSdrInterface->SetSdrBandwidthIndex(m_BandwidthIndex);
		quint32 maxspan = m_pSdrInterface->GetMaxBWFromIndex(m_BandwidthIndex);
		ui->SpanspinBox->setMaximum(maxspan/1000);
		if(m_SpanFrequency>maxspan)
			m_SpanFrequency = maxspan;
		ui->SpanspinBox->setValue(m_SpanFrequency/1000);
		ui->framePlot->SetSpanFreq( m_SpanFrequency );
		//limit demod frequency to Center Frequency +/-span frequency
		ui->frameDemodFreqCtrl->Setup(9, m_CenterFrequency-m_SpanFrequency/2,
									  m_CenterFrequency+m_SpanFrequency/2,
									  1,
									  UNITS_KHZ );
		ui->frameDemodFreqCtrl->SetFrequency(m_DemodFrequency);
		m_pSdrInterface->SetDemod(m_DemodMode, m_DemodSettings[m_DemodMode]);
	}
}

/////////////////////////////////////////////////////////////////////
// Menu Bar action item handler.
//Network Setup Menu
/////////////////////////////////////////////////////////////////////
void MainWindow::OnNetworkDlg()
{
CEditNetDlg dlg(this);
	dlg.m_IPAdr = m_IPAdr;
	dlg.m_Port = m_Port;
	dlg.m_ActiveDevice = m_ActiveDevice;
	dlg.InitDlg();
	if( dlg.exec() )
	{
		if(	dlg.m_DirtyFlag )
		{
			if(CSdrInterface::RUNNING == m_Status)
			{
				m_pSdrInterface->StopSdr();
				ui->framePlot->SetRunningState(false);
			}
			m_IPAdr = dlg.m_IPAdr;
			m_Port = dlg.m_Port;
			m_ActiveDevice = dlg.m_ActiveDevice;
			m_pSdrInterface->SetupNetwork(m_IPAdr,m_Port);
		}
	}
}

/////////////////////////////////////////////////////////////////////
// Menu Bar action item handler.
//Demod Setup Menu (Non-Modal ie allows user to continue in other windows)
/////////////////////////////////////////////////////////////////////
void MainWindow::OnDemodDlg()
{
	m_pDemodSetupDlg->m_DemodMode = m_DemodMode;
	m_pDemodSetupDlg->InitDlg();
	m_pDemodSetupDlg->show();
}

/////////////////////////////////////////////////////////////////////
// Menu Bar action item handler.
//Noise Processing Setup Menu
/////////////////////////////////////////////////////////////////////
void MainWindow::OnNoiseProcDlg()
{
CNoiseProcDlg dlg(this);
	dlg.InitDlg(&m_NoiseProcSettings);
	dlg.exec();
}


/////////////////////////////////////////////////////////////////////
// Called to update the information text box
/////////////////////////////////////////////////////////////////////
void MainWindow::UpdateInfoBox()
{
	//display filter cutoffs
	m_Str = QString("%1 %2 %3 %4 %5")
			.arg(m_DemodSettings[m_DemodMode].txt)
			.arg("Lo=").arg(m_DemodSettings[m_DemodMode].LowCut)
			.arg("Hi=").arg(m_DemodSettings[m_DemodMode].HiCut);
	ui->InfoText->setText(m_Str);
}

/////////////////////////////////////////////////////////////////////
// Called by Start/Stop Button event
/////////////////////////////////////////////////////////////////////
void MainWindow::OnRun()
{
	if( CSdrInterface::CONNECTED == m_Status)
	{
		m_CenterFrequency = m_pSdrInterface->SetRxFreq(m_CenterFrequency);
		m_pSdrInterface->SetDemodFreq(m_CenterFrequency - m_DemodFrequency);
		m_pSdrInterface->StartSdr();
		m_pSdrInterface->m_MissedPackets = 0;

		ui->framePlot->SetRunningState(true);
		InitPerformance();
	}
	else if(CSdrInterface::RUNNING == m_Status)
	{
		m_pSdrInterface->StopSdr();
		ui->framePlot->SetRunningState(false);
		ReadPerformance();
	}
}

/////////////////////////////////////////////////////////////////////
// Event: New FFT Display Data is available so draw it
// Called when new spectrum data is available to be displayed
/////////////////////////////////////////////////////////////////////
void MainWindow::OnNewFftData()
{
	if( CSdrInterface::RUNNING == m_Status)
		ui->framePlot->draw();
}

/////////////////////////////////////////////////////////////////////
// Manage Status states. Called periodically by Timer event
/////////////////////////////////////////////////////////////////////
void MainWindow::OnStatus(int status)
{
	m_Status = (CSdrInterface::eStatus)status;
//qDebug()<<"Status"<< status;
	switch(status)
	{
		case CSdrInterface::NOT_CONNECTED:
			if(	m_LastStatus == CSdrInterface::RUNNING)
			{
				m_pSdrInterface->StopSdr();
				ui->framePlot->SetRunningState(false);
			}
			ui->statusBar->showMessage(tr("SDR Not Connected"), 0);
			ui->pushButtonRun->setText("Run");
			ui->pushButtonRun->setEnabled(false);
			break;
		case CSdrInterface::CONNECTED:
			if(	m_LastStatus == CSdrInterface::RUNNING)
			{
				m_pSdrInterface->StopSdr();
				ui->framePlot->SetRunningState(false);
			}
			ui->statusBar->showMessage( m_ActiveDevice + tr(" Connected"), 0);
			if(	m_LastStatus == CSdrInterface::NOT_CONNECTED)
				m_pSdrInterface->GetSdrInfo();
			ui->pushButtonRun->setText("Run");
			ui->pushButtonRun->setEnabled(true);
			break;
		case CSdrInterface::RUNNING:
			m_Str.setNum(m_pSdrInterface->GetRateError());
			m_Str.append(" ppm  Missed Pkts=");
			m_Str2.setNum(m_pSdrInterface->m_MissedPackets);
			m_Str.append(m_Str2);
			ui->statusBar->showMessage(m_ActiveDevice + tr(" Running   ") + m_Str, 0);
			ui->pushButtonRun->setText("Stop");
			ui->pushButtonRun->setEnabled(TRUE);
			break;
		case CSdrInterface::ERROR:
			if(	m_LastStatus == CSdrInterface::RUNNING)
			{
				m_pSdrInterface->StopSdr();
				ui->framePlot->SetRunningState(false);
			}
			ui->statusBar->showMessage(tr("SDR Not Connected"), 0);
			ui->pushButtonRun->setText("Run");
			ui->pushButtonRun->setEnabled(FALSE);
			break;
		case CSdrInterface::ADOVR:
			if(	m_LastStatus == CSdrInterface::RUNNING)
			{
				m_Status = CSdrInterface::RUNNING;
				ui->framePlot->SetADOverload(true);
			}
			break;
		default:
			break;
	}
	m_LastStatus = m_Status;
}

/////////////////////////////////////////////////////////////////////
// Event handler that New SDR Info is available
// Called when a radio is first conencted and it reports its information
/////////////////////////////////////////////////////////////////////
void MainWindow::OnNewInfoData()
{
	m_ActiveDevice = m_pSdrInterface->m_DeviceName;
	m_RadioType = m_pSdrInterface->GetRadioType();
	m_pSdrInterface->SetSdrBandwidthIndex(m_BandwidthIndex);

	//update span limits to new radio attached
	quint32 maxspan = m_pSdrInterface->GetMaxBWFromIndex(m_BandwidthIndex);
	ui->SpanspinBox->setMaximum(maxspan/1000);
	if(m_SpanFrequency>maxspan)
		m_SpanFrequency = maxspan;
	ui->SpanspinBox->setValue(m_SpanFrequency/1000);
	m_LastSpanKhz = m_SpanFrequency/1000;
	ui->framePlot->SetSpanFreq( m_SpanFrequency );
	m_pSdrInterface->SetDemod(m_DemodMode, m_DemodSettings[m_DemodMode]);
}

/////////////////////////////////////////////////////////////////////
// Handle change event for center frequency control
/////////////////////////////////////////////////////////////////////
void MainWindow::OnNewCenterFrequency(qint64 freq)
{
//qDebug()<<"F = "<<freq;
	m_CenterFrequency = m_pSdrInterface->SetRxFreq(freq);
	if(m_CenterFrequency!=freq)	//if freq was clamped by sdr range then update control again
		ui->frameFreqCtrl->SetFrequency(m_CenterFrequency);
	m_DemodFrequency = m_CenterFrequency;
	m_pSdrInterface->SetDemodFreq(m_CenterFrequency - m_DemodFrequency);
	ui->framePlot->SetCenterFreq( m_CenterFrequency );
	ui->framePlot->SetDemodCenterFreq( m_DemodFrequency );
	//limit demod frequency to Center Frequency +/-span frequency
	ui->frameDemodFreqCtrl->Setup(9, m_CenterFrequency-m_SpanFrequency/2,
								  m_CenterFrequency+m_SpanFrequency/2,
								  1,
								  UNITS_KHZ );
	ui->frameDemodFreqCtrl->SetFrequency(m_DemodFrequency);
	ui->framePlot->UpdateOverlay();
}

/////////////////////////////////////////////////////////////////////
// Handle change event for demod frequency control
/////////////////////////////////////////////////////////////////////
void MainWindow::OnNewDemodFrequency(qint64 freq)
{
	m_DemodFrequency = freq;
	ui->framePlot->SetDemodCenterFreq( m_DemodFrequency );
	ui->framePlot->UpdateOverlay();
	m_pSdrInterface->SetDemodFreq(m_CenterFrequency - m_DemodFrequency);
}

/////////////////////////////////////////////////////////////////////
// Handle plot mouse change event for center frequency
/////////////////////////////////////////////////////////////////////
void MainWindow::OnNewScreenCenterFreq(qint64 freq)
{
	m_CenterFrequency = freq;
	ui->frameFreqCtrl->SetFrequency(m_CenterFrequency);
}


/////////////////////////////////////////////////////////////////////
// Handle plot mouse change event for demod frequency
/////////////////////////////////////////////////////////////////////
void MainWindow::OnNewScreenDemodFreq(qint64 freq)
{
	m_DemodFrequency = freq;
	ui->frameDemodFreqCtrl->SetFrequency(m_DemodFrequency);
}

/////////////////////////////////////////////////////////////////////
// Handle plot mouse change event for filter cutoff frequencies
/////////////////////////////////////////////////////////////////////
void MainWindow::OnNewLowCutFreq(int freq)
{
	m_DemodSettings[m_DemodMode].LowCut = freq;
	UpdateInfoBox();
	m_pSdrInterface->SetDemod(m_DemodMode, m_DemodSettings[m_DemodMode]);
}

void MainWindow::OnNewHighCutFreq(int freq)
{
	m_DemodSettings[m_DemodMode].HiCut = freq;
	UpdateInfoBox();
	m_pSdrInterface->SetDemod(m_DemodMode, m_DemodSettings[m_DemodMode]);
}


/////////////////////////////////////////////////////////////////////
// Handle Span Spin Control change event
/////////////////////////////////////////////////////////////////////
void MainWindow::OnSpanChanged(int spanKhz)
{
	if( (spanKhz>m_LastSpanKhz) && (spanKhz==10))
	{//if going higher and is 10KHz
		//change stepsize to 10KHz
		ui->SpanspinBox->setSingleStep(10);
	}
	else if( (spanKhz<m_LastSpanKhz) && (spanKhz==10))
	{	//if going lower and is 10KHz
		//change stepsize to 1KHz
		ui->SpanspinBox->setSingleStep(1);
	}
	m_LastSpanKhz = spanKhz;
	m_SpanFrequency = spanKhz*1000;
	ui->framePlot->SetSpanFreq(m_SpanFrequency);
	ui->framePlot->UpdateOverlay();
	//limit demod frequency to Center Frequency +/-span frequency
	ui->frameDemodFreqCtrl->Setup(9, m_CenterFrequency-m_SpanFrequency/2,
								  m_CenterFrequency+m_SpanFrequency/2,
								  1,
								  UNITS_KHZ );
	ui->frameDemodFreqCtrl->SetFrequency(m_DemodFrequency);

}

/////////////////////////////////////////////////////////////////////
// Handle Max dB Spin Control change event
/////////////////////////////////////////////////////////////////////
void MainWindow::OnMaxdBChanged(int maxdb)
{
	m_MaxdB = maxdb;
	ui->framePlot->SetMaxdB(m_MaxdB);
	ui->framePlot->UpdateOverlay();
}

/////////////////////////////////////////////////////////////////////
// Handle Vertical scale COmbo box Control change event
/////////////////////////////////////////////////////////////////////
void MainWindow::OnVertScaleChanged(int index)
{
	//hack to ignor event when control is initialized
	if(ui->ScalecomboBox->count() != 4)
		return;
	m_VertScaleIndex = index;
	int LastdBStepsize = m_dBStepSize;
	int LastMaxdB = m_MaxdB;
	m_dBStepSize = (int)ui->ScalecomboBox->itemData(m_VertScaleIndex).toInt();
	ui->framePlot->SetdBStepSize(m_dBStepSize);
	ui->MaxdBspinBox->setSingleStep(m_dBStepSize);
	ui->MaxdBspinBox->setMinimum(MIN_FFTDB+VERT_DIVS*m_dBStepSize);
	ui->MaxdBspinBox->setMaximum(MAX_FFTDB);

	//adjust m_MaxdB to try and keep signal roughly centered at bottom of screen
	if(m_dBStepSize!=LastdBStepsize)
	{
		m_MaxdB = LastMaxdB + 11*(m_dBStepSize-LastdBStepsize);
		m_MaxdB = (m_MaxdB/m_dBStepSize)*m_dBStepSize;
	}

	ui->MaxdBspinBox->setValue(m_MaxdB);
	ui->framePlot->SetMaxdB(m_MaxdB);
	ui->framePlot->UpdateOverlay();

//qDebug()<<"m_VertScaleIndex="<<m_VertScaleIndex;
//qDebug()<<"dBstep="<<m_dBStepSize;
}

void MainWindow::OnVolumeSlider(int value)
{
	m_Volume = value;
	m_pSdrInterface->SetVolume(m_Volume);
}

/////////////////////////////////////////////////////////////////////
// Setup Noise Processor parameters from 'm_NoiseProcSettings'
/////////////////////////////////////////////////////////////////////
void MainWindow::SetupNoiseProc()
{
	m_pSdrInterface->SetupNoiseProc(&m_NoiseProcSettings);
}

/////////////////////////////////////////////////////////////////////
// Setup Demod parameters and clamp to limits
/////////////////////////////////////////////////////////////////////
void MainWindow::SetupDemod(int index)
{
	m_DemodMode = index;
	ui->framePlot->SetDemodRanges(
			m_DemodSettings[m_DemodMode].LowCutmin,
			m_DemodSettings[m_DemodMode].LowCutmax,
			m_DemodSettings[m_DemodMode].HiCutmin,
			m_DemodSettings[m_DemodMode].HiCutmax,
			m_DemodSettings[m_DemodMode].Symetric);
	//Clamp cutoff freqs to range of particular demod
	if(m_DemodSettings[index].LowCut < m_DemodSettings[index].LowCutmin)
		m_DemodSettings[index].LowCut = m_DemodSettings[index].LowCutmin;
	if(m_DemodSettings[index].LowCut > m_DemodSettings[index].LowCutmax)
		m_DemodSettings[index].LowCut = m_DemodSettings[index].LowCutmax;

	if(m_DemodSettings[index].HiCut < m_DemodSettings[index].HiCutmin)
		m_DemodSettings[index].HiCut = m_DemodSettings[index].HiCutmin;
	if(m_DemodSettings[index].HiCut > m_DemodSettings[index].HiCutmax)
		m_DemodSettings[index].HiCut = m_DemodSettings[index].HiCutmax;

	ui->framePlot->SetHiLowCutFrequencies(m_DemodSettings[m_DemodMode].LowCut,
										  m_DemodSettings[m_DemodMode].HiCut);
	ui->framePlot->SetFilterClickResolution(m_DemodSettings[m_DemodMode].FilterClickResolution);
	ui->framePlot->UpdateOverlay();
	m_pSdrInterface->SetDemod(m_DemodMode, m_DemodSettings[m_DemodMode]);
	m_pSdrInterface->SetDemodFreq(m_CenterFrequency - m_DemodFrequency);
	UpdateInfoBox();
}


/////////////////////////////////////////////////////////////////////
// Setup Demod initial parameters/limits
/////////////////////////////////////////////////////////////////////
void MainWindow::InitDemodSettings()
{
	//set filter limits based on final sample rates etc.
	//These parameters are fixed and not saved in Settings
	m_DemodSettings[DEMOD_AM].txt = "AM";
	m_DemodSettings[DEMOD_AM].HiCutmin = 500;
	m_DemodSettings[DEMOD_AM].HiCutmax = 10000;
	m_DemodSettings[DEMOD_AM].LowCutmax = -500;
	m_DemodSettings[DEMOD_AM].LowCutmin = -10000;
	m_DemodSettings[DEMOD_AM].Symetric = true;

	m_DemodSettings[DEMOD_SAM].txt = "AM";
	m_DemodSettings[DEMOD_SAM].HiCutmin = 100;
	m_DemodSettings[DEMOD_SAM].HiCutmax = 10000;
	m_DemodSettings[DEMOD_SAM].LowCutmax = -100;
	m_DemodSettings[DEMOD_SAM].LowCutmin = -10000;
	m_DemodSettings[DEMOD_SAM].Symetric = false;

	m_DemodSettings[DEMOD_FM].txt = "FM";
	m_DemodSettings[DEMOD_FM].HiCutmin = 5000;
	m_DemodSettings[DEMOD_FM].HiCutmax = 15000;
	m_DemodSettings[DEMOD_FM].LowCutmax = -5000;
	m_DemodSettings[DEMOD_FM].LowCutmin = -15000;
	m_DemodSettings[DEMOD_FM].Symetric = true;

	m_DemodSettings[DEMOD_USB].txt = "USB";
	m_DemodSettings[DEMOD_USB].HiCutmin = 500;
	m_DemodSettings[DEMOD_USB].HiCutmax = 20000;
	m_DemodSettings[DEMOD_USB].LowCutmax = 200;
	m_DemodSettings[DEMOD_USB].LowCutmin = 0;
	m_DemodSettings[DEMOD_USB].Symetric = false;

	m_DemodSettings[DEMOD_LSB].txt = "LSB";
	m_DemodSettings[DEMOD_LSB].HiCutmin = -200;
	m_DemodSettings[DEMOD_LSB].HiCutmax = 0;
	m_DemodSettings[DEMOD_LSB].LowCutmax = -500;
	m_DemodSettings[DEMOD_LSB].LowCutmin = -20000;
	m_DemodSettings[DEMOD_LSB].Symetric = false;

	m_DemodSettings[DEMOD_CWU].txt = "CWU";
	m_DemodSettings[DEMOD_CWU].HiCutmin = 50;
	m_DemodSettings[DEMOD_CWU].HiCutmax = 1000;
	m_DemodSettings[DEMOD_CWU].LowCutmax = -50;
	m_DemodSettings[DEMOD_CWU].LowCutmin = -1000;
	m_DemodSettings[DEMOD_CWU].Symetric = false;

	m_DemodSettings[DEMOD_CWL].txt = "CWL";
	m_DemodSettings[DEMOD_CWL].HiCutmin = 50;
	m_DemodSettings[DEMOD_CWL].HiCutmax = 1000;
	m_DemodSettings[DEMOD_CWL].LowCutmax = -50;
	m_DemodSettings[DEMOD_CWL].LowCutmin = -1000;
	m_DemodSettings[DEMOD_CWL].Symetric = false;

	m_pSdrInterface->SetDemod(m_DemodMode, m_DemodSettings[m_DemodMode]);
}
