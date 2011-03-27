//////////////////////////////////////////////////////////////////////
// testbench.cpp: implementation of the CTestBench class.
//
//  This class creates a test bench dialog that generates complex
// signals and displays complex data and spectrum for testing and debug
//
// History:
//	2010-12-18  Initial creation MSW
//	2011-03-27  Initial release
//////////////////////////////////////////////////////////////////////

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
#include "testbench.h"
#include "ui_testbench.h"
#include <QDebug>

CTestBench* g_pTestBench = NULL;		//pointer to this class is global so everybody can access

#define USE_FILE 0
//#define FILE_NAME "SSB-7210000Hz_001.wav"
#define FILE_NAME "sim5.wav"
#define USE_SVFILE 0
#define USE_PERSEUSFILE 1

//////////////////////////////////////////////////////////////////////
// Local Defines
//////////////////////////////////////////////////////////////////////
#define MAX_AMPLITUDE 32767.0
#define TESTFFT_SIZE 2048

#define TRIG_OFF 0
#define TRIG_PNORM 1
#define TRIG_PSINGLE 2
#define TRIG_NNORM 3
#define TRIG_NSINGLE 4

#define TRIGSTATE_WAIT 0
#define TRIGSTATE_CAPTURE 1
#define TRIGSTATE_DISPLAY 2
#define TRIGSTATE_WAITDISPLAY 3


//list of defined profiles
const char* PROF_STR[NUM_PROFILES] =
{
	"Off",
	"PreFilter",
	"PostFilter",
	"PostAGC",
	"PostDemod",
	"Soundcard",
	"Profile 6",
	"Noise Blanker"
};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CTestBench::CTestBench(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CTestBench)
{
	m_Active = false;
	m_2DPixmap = QPixmap(0,0);
	m_OverlayPixmap = QPixmap(0,0);
	m_Size = QSize(0,0);
	m_Rect = QRect(0,0,100,100);

	m_MaxdB = 10;
	m_MindB = -120;
	m_dBStepSize = 10;
	m_FreqUnits = 1;
	m_CenterFreq = 0;
	m_GenSampleRate = 1;
	m_DisplaySampleRate = 1;
	m_Span = m_DisplaySampleRate/2;
	m_FftBufPos = 0;

	m_GenOn = false;
	m_PeakOn = false;
	m_NewDataIsCpx = false;
	m_CurrentDataIsCpx = true;
	m_TimeDisplay = false;
	m_DisplayRate = 10;
	m_HorzSpan = 100;
	m_VertRange = 65000;
	m_TrigLevel = 100;
	m_TrigBufPos = 0;
	m_TrigCounter = 0;
	m_TrigState = TRIGSTATE_WAIT;

	m_PulseWidth = .01;
	m_PulsePeriod = .5;
	m_PulseTimer = 0.0;

	connect(this, SIGNAL(ResetSignal()), this,  SLOT( Reset() ) );
	connect(this, SIGNAL(NewFftData()), this,  SLOT( DrawFftPlot() ) );
	connect(this, SIGNAL(NewTimeData()), this,  SLOT( DrawTimePlot() ) );
	connect( this, SIGNAL( SendTxt(QString)), this, SLOT( GotTxt(QString) ) );

	m_Fft.SetFFTParams( 2048,
						FALSE,
						0.0,
						m_GenSampleRate);
	m_Fft.SetFFTAve(0);
	ui->setupUi(this);
	setWindowTitle("CuteSDR Test Bench");
	ui->textEdit->clear();
	m_pTimer = new QTimer(this);
	connect(m_pTimer, SIGNAL(timeout()), this, SLOT(OnTimer()));

#if USE_FILE	//test file reading kludge
	QDir::setCurrent("d:/");
	m_File.setFileName(FILE_NAME);
	if(m_File.open(QIODevice::ReadOnly))
	{
		qDebug()<<"file Opend OK";
		if(USE_SVFILE)
			m_File.seek(0x7e);		//SV
		else if(USE_PERSEUSFILE)
			m_File.seek(0x7A);		//perseus
	}
	else
		qDebug()<<"file Failed to Open";
#endif
}

CTestBench::~CTestBench()
{
	if(m_File.isOpen())
		m_File.close();
    delete ui;
}

//////////////////////////////////////////////////////////////////////
//  Handle window close and show events
//////////////////////////////////////////////////////////////////////
void CTestBench::closeEvent(QCloseEvent *event)
{
	Q_UNUSED(event);
	m_Active = false;
	m_pTimer->stop();		//stop timer
}

void CTestBench::showEvent(QShowEvent *event)
{
	Q_UNUSED(event);
	m_Active = true;
	m_pTimer->start(500);		//start up timer
}

//////////////////////////////////////////////////////////////////////
// Called by parent to Initialize testbench controls after persistent
// variables are initialized
//////////////////////////////////////////////////////////////////////
void CTestBench::Init()
{
	int tmp = m_TrigIndex;	//save since adding items changes m_TrigIndex
	ui->comboBoxTrig->clear();
	ui->comboBoxTrig->addItem("Free Run");
	ui->comboBoxTrig->addItem("Pos Edge");
	ui->comboBoxTrig->addItem("Pos Single");
	ui->comboBoxTrig->addItem("Neg Edge");
	ui->comboBoxTrig->addItem("Neg Single");
	m_TrigIndex = tmp;
	ui->comboBoxTrig->setCurrentIndex(m_TrigIndex);


	tmp = m_Profile;
	for(int i=0; i<NUM_PROFILES; i++)
		ui->comboBoxProfile->addItem(PROF_STR[i],i);
	m_Profile = tmp;
	ui->comboBoxProfile->setCurrentIndex(m_Profile);

	ui->spinBoxStart->setValue(m_SweepStartFrequency/1000.0);
	ui->spinBoxStop->setValue(m_SweepStopFrequency/1000.0);
	ui->spinBoxSweep->setValue(m_SweepRate);
	ui->checkBoxTime->setChecked(m_TimeDisplay);
	ui->spinBoxHorzSpan->setValue(m_HorzSpan);

	ui->spinBoxThresh->setValue(m_TrigLevel);
	ui->spinBoxVertRange->setValue(m_VertRange);
	ui->spinBoxRate->setValue(m_DisplayRate);

	ui->checkBoxGen->setChecked(m_GenOn);
	ui->checkBoxPeak->setChecked(m_PeakOn);
	ui->spinBoxAmp->setValue((int)m_SignalPower);
	ui->spinBoxNoise->setValue((int)m_NoisePower);
	ui->spinBoxPulseWidth->setValue((int)(1000.0*m_PulseWidth));
	ui->spinBoxPulsePeriod->setValue((int)(1000.0*m_PulsePeriod));

	Reset();
}

//////////////////////////////////////////////////////////////////////
// Bunch of Control Slot functions called when a control value changes
//////////////////////////////////////////////////////////////////////
void CTestBench::OnSweepStart(int start)
{
	m_SweepStartFrequency = (double)start*1000.0;
	m_SweepFrequency = m_SweepStartFrequency;
	m_SweepAcc = 0.0;
}

void CTestBench::OnSweepStop(int stop)
{
	m_SweepStopFrequency = (double)stop*1000.0;
	m_SweepFrequency = m_SweepStartFrequency;
	m_SweepAcc = 0.0;
}

void CTestBench::OnSweepRate(int rate)
{
	m_SweepRate = (double)rate; // Hz/sec
	m_SweepAcc = 0.0;
	m_SweepRateInc = m_SweepRate/m_GenSampleRate;
}


void CTestBench::OnDisplayRate(int rate)
{
	m_DisplayRate = rate;
	if(m_TimeDisplay)
	{
		double capturesize = ( (double)m_HorzSpan*m_DisplaySampleRate/1000.0);
		m_DisplaySkipValue = m_DisplaySampleRate/(capturesize*m_DisplayRate);
	}
	else
	{
		m_DisplaySkipValue = m_DisplaySampleRate/(TEST_FFTSIZE*m_DisplayRate);
	}
}

void CTestBench::OnVertRange(int range )
{
	m_VertRange = range;
	ui->spinBoxThresh->setMaximum(m_VertRange/2);
	ui->spinBoxThresh->setMinimum(-m_VertRange/2);
	if(m_TimeDisplay)
		DrawTimeOverlay();
}

void CTestBench::OnHorzSpan(int span)
{
	m_HorzSpan = span;
	if(m_TimeDisplay)
	{
		DrawTimeOverlay();
		double capturesize = ( (double)m_HorzSpan*m_DisplaySampleRate/1000.0);
		m_DisplaySkipValue = m_DisplaySampleRate/(capturesize*m_DisplayRate);
		m_TimeScrnPixel = .001* (double)((double)m_HorzSpan/(double)m_Rect.width());	//time per pixel on screen in seconds
	}
}

void CTestBench::OnTimeDisplay(bool timemode)
{
	m_TimeDisplay = timemode;
	Reset();
}

void CTestBench::OnTriggerMode(int trigindex)
{
	m_TrigIndex = trigindex;
	Reset();
}

void CTestBench::OnTrigLevel(int level)
{
	m_TrigLevel = level;
	if(m_TimeDisplay)
		DrawTimeOverlay();
}

void CTestBench::OnProfile(int profindex)
{
	m_Profile = profindex;

}

void CTestBench::OnGenOn(bool On)
{
	m_GenOn = On;
}

void CTestBench::OnPulseWidth(int pwidth)
{
	m_PulseWidth = (double)pwidth * .001;
}

void CTestBench::OnPulsePeriod(int pperiod)
{
	m_PulsePeriod = (double)pperiod * .001;
}

void CTestBench::OnSignalPwr(int pwr)
{
	m_SignalPower = pwr;
	m_SignalAmplitude = MAX_AMPLITUDE*pow(10.0, m_SignalPower/20.0);
}

void CTestBench::OnNoisePwr(int pwr)
{
	m_NoisePower = pwr;
	m_NoiseAmplitude = MAX_AMPLITUDE*pow(10.0, m_NoisePower/20.0);
}

void CTestBench::OnEnablePeak(bool enablepeak)
{
	for( int i=0; i<TB_MAX_SCREENSIZE; i++)
	{
		m_FftPkBuf[i] = m_Rect.height();	//set pk buffer to minimum screen posistion
		m_TimeBuf1[i] = 0;
		m_TimeBuf2[i] = 0;
	}
	m_PeakOn = enablepeak;
}



//////////////////////////////////////////////////////////////////////
// Call to Create 'length' complex sweep/pulse/noise generator samples
//and place in users pBuf.
//Called by thread so no GUI calls!
//////////////////////////////////////////////////////////////////////
void CTestBench::CreateGeneratorSamples(int length, TYPECPX* pBuf, double samplerate)
{
int i;
double rad;
double r;
double u1;
double u2;
	if(!m_Active || !m_GenOn)
		return;
	if(m_GenSampleRate != samplerate)
	{	//reset things if sample rate changes on the fly
		m_GenSampleRate = samplerate;
		emit ResetSignal();
	}

#if USE_FILE	//test file reading kludge
	char buf[16384];
	if(m_File.atEnd())
	{
		if(USE_SVFILE)
			m_File.seek(0x7e);		//SV
		else if(USE_PERSEUSFILE)
			m_File.seek(0x7A);		//perseus
		return;
	}
	if(m_File.read(buf,6*length)<=0)

		return;
	int j=0;
	for(i=0; i<length; i++)
	{
		tBtoL2 tmp;
		tmp.bytes.b0 = 0;
		tmp.bytes.b1 = buf[j++];
		tmp.bytes.b2 = buf[j++];
		tmp.bytes.b3 = buf[j++];
		pBuf[i].re = (TYPEREAL)tmp.all/65536;
		tmp.bytes.b1 = buf[j++];
		tmp.bytes.b2 = buf[j++];
		tmp.bytes.b3 = buf[j++];
		pBuf[i].im = (TYPEREAL)tmp.all/65536;
	}
	return;
#endif
	for(i=0; i<length; i++)
	{
		double amp = m_SignalAmplitude;
		if(m_PulseWidth > 0.0)
		{	//if pulse width is >0 create pulse modulation
			m_PulseTimer += (1.0/m_GenSampleRate);
			if(m_PulseTimer > m_PulsePeriod)
				m_PulseTimer = 0.0;
			if(m_PulseTimer > m_PulseWidth)
				amp = 0.0;
		}

#if 0		//way to skip over passband for filter alias testing
if( (m_SweepFrequency>-31250) && (m_SweepFrequency<31250) )
{
	m_SweepFrequency = 31250;
	amp = 0.0;
	m_Fft.ResetFFT();
	m_DisplaySkipCounter = -2;
}
#endif

		//create complex sin/cos signal
		pBuf[i].re = amp*cos(m_SweepAcc);
		pBuf[i].im = amp*sin(m_SweepAcc);
		//inc phase accummulator with normalized freqeuency step


		m_SweepAcc += ( m_SweepFrequency*m_SweepFreqNorm );
		m_SweepFrequency += m_SweepRateInc;	//inc sweep frequency
		if(m_SweepFrequency >= m_SweepStopFrequency)	//reached end of sweep?
			m_SweepRateInc = 0.0;						//stop sweep when end is reached
//			m_SweepFrequency = m_SweepStartFrequency;	//restart sweep when end is reached

		//////////////////  Gaussian Noise generator
		// Generate two uniform random numbers between -1 and +1
		// that are inside the unit circle
		if(m_NoisePower > -160.0)
		{	//create and add noise samples to signal
			do {
				u1 = 1.0 - 2.0 * (double)rand()/(double)RAND_MAX ;
				u2 = 1.0 - 2.0 * (double)rand()/(double)RAND_MAX ;
				r = u1*u1 + u2*u2;
			} while(r >= 1.0 || r == 0.0);
			rad = sqrt(-2.0*log(r)/r);
			//add noise samples to generator output
			pBuf[i].re += (m_NoiseAmplitude*u1*rad);
			pBuf[i].im += (m_NoiseAmplitude*u2*rad);
		}
	}
	m_SweepAcc = (double)fmod((double)m_SweepAcc, K_2PI);	//keep radian counter bounded
}

//////////////////////////////////////////////////////////////////////
// Call to Create 'length' real sweep/pulse/noise generator samples
//and place in users pBuf.
//Called by thread so no GUI calls!
//////////////////////////////////////////////////////////////////////
void CTestBench::CreateGeneratorSamples(int length, TYPEREAL* pBuf, double samplerate)
{
int i;
double rad;
double r;
double u1;
double u2;
	if(!m_Active || !m_GenOn)
		return;
	if(m_GenSampleRate != samplerate)
	{	//reset things if sample rate changes on the fly
		m_GenSampleRate = samplerate;
		emit ResetSignal();
	}
	for(i=0; i<length; i++)
	{
		double amp = m_SignalAmplitude;
		if(m_PulseWidth > 0.0)
		{	//if pulse width is >0 create pulse modulation
			m_PulseTimer += (1.0/m_GenSampleRate);
			if(m_PulseTimer > m_PulsePeriod)
				m_PulseTimer = 0.0;
			if(m_PulseTimer > m_PulseWidth)
				amp = 0.0;
		}

#if 0		//way to skip over passband for filter alias testing
if( (m_SweepFrequency>-31250) && (m_SweepFrequency<31250) )
{
	m_SweepFrequency = 31250;
	amp = 0.0;
	m_Fft.ResetFFT();
	m_DisplaySkipCounter = -2;
}
#endif

		//create cos signal
		pBuf[i] = 3.0*amp*cos(m_SweepAcc);
		//inc phase accummulator with normalized freqeuency step


		m_SweepAcc += ( m_SweepFrequency*m_SweepFreqNorm );
		m_SweepFrequency += m_SweepRateInc;	//inc sweep frequency
		if(m_SweepFrequency >= m_SweepStopFrequency)	//reached end of sweep?
			m_SweepRateInc = 0.0;						//stop sweep when end is reached
//			m_SweepFrequency = m_SweepStartFrequency;	//restart sweep when end is reached

		//////////////////  Gaussian Noise generator
		// Generate two uniform random numbers between -1 and +1
		// that are inside the unit circle
		if(m_NoisePower > -160.0)
		{	//create and add noise samples to signal
			do {
				u1 = 1.0 - 2.0 * (double)rand()/(double)RAND_MAX ;
				u2 = 1.0 - 2.0 * (double)rand()/(double)RAND_MAX ;
				r = u1*u1 + u2*u2;
			} while(r >= 1.0 || r == 0.0);
			rad = sqrt(-2.0*log(r)/r);
			//add noise samples to generator output
			pBuf[i] += (m_NoiseAmplitude*u1*rad);
		}
	}
	m_SweepAcc = (double)fmod((double)m_SweepAcc, K_2PI);	//keep radian counter bounded
}

//////////////////////////////////////////////////////////////////////
// Called to reset the generator and display peaks
//recalculates a bunch of variables based on settings
//////////////////////////////////////////////////////////////////////
void CTestBench::Reset()
{
int i;
	//initialize sweep generator values
	m_SweepFrequency = m_SweepStartFrequency;
	m_SweepFreqNorm = K_2PI/m_GenSampleRate;
	m_SweepAcc = 0.0;
	m_SweepRateInc = m_SweepRate/m_GenSampleRate;
	m_SignalAmplitude = MAX_AMPLITUDE*pow(10.0, m_SignalPower/20.0);
	m_NoiseAmplitude = MAX_AMPLITUDE*pow(10.0, m_NoisePower/20.0);

	//init FFT values
	m_Fft.SetFFTParams(  TEST_FFTSIZE, FALSE, 0.0,	m_DisplaySampleRate);
	m_FftBufPos = 0;
	m_Span = m_DisplaySampleRate;
	m_Span = ( m_Span - (m_Span+5)%10 + 5);

	//init time display values
	m_TimeScrnPixel = .001* (double)((double)m_HorzSpan/(double)m_Rect.width());	//time per pixel on screen in seconds


	m_TimeScrnPos = 0;
	m_TimeInPos = 0;
	m_PreviousSample = 0;
	m_PostScrnCaptureLength = (7*m_Rect.width())/10;	//sets trigger position
	m_TrigState = TRIGSTATE_WAIT;

	for(i=0; i<TEST_FFTSIZE; i++)
	{
		m_FftInBuf[i].re = 0.0;
		m_FftInBuf[i].im = 0.0;
	}
	for( i=0; i<TB_MAX_SCREENSIZE; i++)
	{
		m_FftPkBuf[i] = m_Rect.height();	//set pk buffer to minimum screen posistion
		m_TimeBuf1[i] = 0;
		m_TimeBuf2[i] = 0;
	}
	if(m_TimeDisplay)
	{
		DrawTimeOverlay();
		double capturesize = ( (double)m_HorzSpan*m_DisplaySampleRate/1000.0);
		m_DisplaySkipValue = m_DisplaySampleRate/(capturesize*m_DisplayRate);
	}
	else
	{
		DrawFreqOverlay();
		m_DisplaySkipValue = m_DisplaySampleRate/(TEST_FFTSIZE*m_DisplayRate);
	}
	ui->textEdit->clear();
	m_Fft.ResetFFT();
	m_DisplaySkipCounter = -2;
	m_PulseTimer = 0.0;
 }

//////////////////////////////////////////////////////////////////////
// Called to display the input pBuf.
//Called by thread so no GUI calls!
// COMPLEX Data version.
//////////////////////////////////////////////////////////////////////
void CTestBench::DisplayData(int length, TYPECPX* pBuf, double samplerate, int profile)
{
	if(!m_Active || (profile!=m_Profile) )
		return;
	if(m_DisplaySampleRate != samplerate)
	{
		m_DisplaySampleRate = samplerate;
		emit ResetSignal();
		return;
	}
	m_NewDataIsCpx = true;
	if(! m_TimeDisplay)
	{	//if displaying frequency domain data
		//accumulate samples into m_FftInBuf until have enough to perform an FFT
		for(int i=0; i<length; i++)
		{
			m_FftInBuf[m_FftBufPos++] = pBuf[i];
			if(m_FftBufPos >= TEST_FFTSIZE )
			{
				m_FftBufPos = 0;
				if(++m_DisplaySkipCounter >= m_DisplaySkipValue )
				{
					m_DisplaySkipCounter = 0;
					m_Fft.PutInDisplayFFT( TESTFFT_SIZE, m_FftInBuf);
					emit NewFftData();
				}
			}
		}
	}
	else
	{	//if displaying time domain data
		for(int i=0; i<length; i++)
		{
			//calc to time variables, one in sample time units and
			//one in screen pixel units
			double intime = (double)m_TimeInPos/samplerate;
			double scrntime = (double)m_TimeScrnPos*m_TimeScrnPixel;
			m_TimeInPos++;
			while(intime >= scrntime)
			{
				ChkForTrigger( (int)pBuf[i].re );
				m_TimeBuf1[m_TimeScrnPos] = (int)pBuf[i].re;
				m_TimeBuf2[m_TimeScrnPos++] = (int)pBuf[i].im;
				scrntime = (double)m_TimeScrnPos*m_TimeScrnPixel;
				if( m_TimeScrnPos >= m_Rect.width() )
				{
					m_TimeScrnPos = 0;
					m_TimeInPos = 0;
					break;
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Called to display the input pBuf.
//Called by thread so no GUI calls!
// REAL Data version.
//////////////////////////////////////////////////////////////////////
void CTestBench::DisplayData(int length, TYPEREAL* pBuf, double samplerate, int profile)
{
	if(!m_Active || (profile!=m_Profile) )
		return;
	if(m_DisplaySampleRate != samplerate)
	{
		m_DisplaySampleRate = samplerate;
		emit ResetSignal();
		return;
	}
	m_NewDataIsCpx = false;
	if(!m_TimeDisplay)
	{	//if displaying frequency domain data
		//accumulate samples into m_FftInBuf until have enough to perform an FFT
		for(int i=0; i<length; i++)
		{
			m_FftInBuf[m_FftBufPos].re = pBuf[i];
			m_FftInBuf[m_FftBufPos++].im = 0.0;
			if(m_FftBufPos >= TEST_FFTSIZE )
			{
				m_FftBufPos = 0;
				if(++m_DisplaySkipCounter >= m_DisplaySkipValue )
				{
					m_DisplaySkipCounter = 0;
					m_Fft.PutInDisplayFFT( TESTFFT_SIZE, m_FftInBuf);
					emit NewFftData();
				}
			}
		}
	}
	else
	{
		for(int i=0; i<length; i++)
		{
			double intime = (double)m_TimeInPos/samplerate;
			double scrntime = (double)m_TimeScrnPos*m_TimeScrnPixel;
			m_TimeInPos++;
			while(intime >= scrntime)
			{
				ChkForTrigger( (int)pBuf[i] );
				m_TimeBuf1[m_TimeScrnPos] = (int)pBuf[i];
				m_TimeBuf2[m_TimeScrnPos++] = 0;
				scrntime = (double)m_TimeScrnPos*m_TimeScrnPixel;
				if( m_TimeScrnPos >= m_Rect.width() )
				{
					m_TimeScrnPos = 0;
					m_TimeInPos = 0;
					break;
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Called to display the input pBuf.
//Called by thread so no GUI calls!
// MONO 16 bit Data version.
//////////////////////////////////////////////////////////////////////
void CTestBench::DisplayData(int length, TYPEMONO16* pBuf, double samplerate, int profile)
{
	if(!m_Active || (profile!=m_Profile) )
		return;
	if(m_DisplaySampleRate != samplerate)
	{
		m_DisplaySampleRate = samplerate;
		emit ResetSignal();
		return;
	}
	m_NewDataIsCpx = false;
	if(!m_TimeDisplay)
	{	//if displaying frequency domain data
		//accumulate samples into m_FftInBuf until have enough to perform an FFT
		for(int i=0; i<length; i++)
		{
			m_FftInBuf[m_FftBufPos].re = (TYPEREAL)pBuf[i];
			m_FftInBuf[m_FftBufPos++].im = (TYPEREAL)pBuf[i];
			if(m_FftBufPos >= TEST_FFTSIZE )
			{
				m_FftBufPos = 0;
				if(++m_DisplaySkipCounter >= m_DisplaySkipValue )
				{
					m_DisplaySkipCounter = 0;
					m_Fft.PutInDisplayFFT( TESTFFT_SIZE, m_FftInBuf);
					emit NewFftData();
				}
			}
		}
	}
	else
	{
		for(int i=0; i<length; i++)
		{
			double intime = (double)m_TimeInPos/samplerate;
			double scrntime = (double)m_TimeScrnPos*m_TimeScrnPixel;
			m_TimeInPos++;
			while(intime >= scrntime)
			{
				ChkForTrigger( (int)pBuf[i<<1] );
				m_TimeBuf1[m_TimeScrnPos] = (int)pBuf[i];
				m_TimeBuf2[m_TimeScrnPos++] = 0;
				scrntime = (double)m_TimeScrnPos*m_TimeScrnPixel;
				if( m_TimeScrnPos >= m_Rect.width() )
				{
					m_TimeScrnPos = 0;
					m_TimeInPos = 0;
					break;
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Called to display the input pBuf.
//Called by thread so no GUI calls!
// STEREO 16 bit Data version.
//////////////////////////////////////////////////////////////////////
void CTestBench::DisplayData(int length, TYPESTEREO16* pBuf, double samplerate, int profile)
{
	if(!m_Active || (profile!=m_Profile) )
		return;
	if(m_DisplaySampleRate != samplerate)
	{
		m_DisplaySampleRate = samplerate;
		emit ResetSignal();
		return;
	}
	m_NewDataIsCpx = true;
	if(! m_TimeDisplay)
	{	//if displaying frequency domain data
		//accumulate samples into m_FftInBuf until have enough to perform an FFT
		for(int i=0; i<length; i++)
		{
			m_FftInBuf[m_FftBufPos].re = (TYPEREAL)pBuf[i].re;
			m_FftInBuf[m_FftBufPos++].im = (TYPEREAL)pBuf[i].im;
			if(m_FftBufPos >= TEST_FFTSIZE )
			{
				m_FftBufPos = 0;
				if(++m_DisplaySkipCounter >= m_DisplaySkipValue )
				{
					m_DisplaySkipCounter = 0;
					m_Fft.PutInDisplayFFT( TESTFFT_SIZE, m_FftInBuf);
					emit NewFftData();
				}
			}
		}
	}
	else
	{	//if displaying time domain data
		for(int i=0; i<length; i++)
		{
			double intime = (double)m_TimeInPos/samplerate;
			double scrntime = (double)m_TimeScrnPos*m_TimeScrnPixel;
			m_TimeInPos++;
			while(intime >= scrntime)
			{
				ChkForTrigger( (int)pBuf[i].re );
				m_TimeBuf1[m_TimeScrnPos] = pBuf[i].re;
				m_TimeBuf2[m_TimeScrnPos++] = pBuf[i].im;
				scrntime = (double)m_TimeScrnPos*m_TimeScrnPixel;
				if( m_TimeScrnPos >= m_Rect.width() )
				{
					m_TimeScrnPos = 0;
					m_TimeInPos = 0;
					break;
				}
			}

		}
	}
}

//////////////////////////////////////////////////////////////////////
// Called to check for O'Scope display trigger condition and manage trigger capture logic.
//////////////////////////////////////////////////////////////////////
void CTestBench::ChkForTrigger(qint32 sample)
{
	switch(m_TrigIndex)
	{
		case TRIG_OFF:
			if( 0 == m_TimeScrnPos )
			{
				if(( ++m_DisplaySkipCounter >= m_DisplaySkipValue ) &&
					  ( m_DisplaySkipCounter > 2) )
				{
					m_DisplaySkipCounter = 0;
					m_TrigBufPos = 0;
					m_TrigState = TRIGSTATE_DISPLAY;
				}
			}
			break;
		case TRIG_PNORM:
		case TRIG_PSINGLE:
				if(TRIGSTATE_WAIT == m_TrigState)
				{
					//look for positive transition through trigger level
					if( (sample>=m_TrigLevel) && (m_PreviousSample<m_TrigLevel) )
					{
						m_TrigBufPos = m_TimeScrnPos;
						m_TrigState = TRIGSTATE_CAPTURE;
						m_TrigCounter = 0;
					}
				}
				else if(TRIGSTATE_CAPTURE == m_TrigState)
				{
					if(++m_TrigCounter >= m_PostScrnCaptureLength )
					{
						m_TrigState = TRIGSTATE_DISPLAY;
						m_TrigCounter = 0;
					}
				}
			break;
		case TRIG_NNORM:
		case TRIG_NSINGLE:
			if(TRIGSTATE_WAIT == m_TrigState)
			{
				//look for negative transition through trigger level
				if( (sample<=m_TrigLevel) && (m_PreviousSample>m_TrigLevel) )
				{
					m_TrigBufPos = m_TimeScrnPos;
					m_TrigState = TRIGSTATE_CAPTURE;
					m_TrigCounter = 0;
				}
			}
			else if(TRIGSTATE_CAPTURE == m_TrigState)
			{
				if(++m_TrigCounter >= m_PostScrnCaptureLength )
				{
					m_TrigState = TRIGSTATE_DISPLAY;
					m_TrigCounter = 0;
				}
			}
			break;
		default:
			break;
	}
	if(TRIGSTATE_DISPLAY == m_TrigState)
	{
		m_TrigState = TRIGSTATE_WAITDISPLAY;
		//copy into screen display buffers
		int w = m_2DPixmap.width();
		int bufpos = m_TrigBufPos + m_PostScrnCaptureLength - w;
		if(bufpos<0)
			bufpos = w + bufpos;
		for(int i=0; i<w; i++)
		{	//copy into display buffers then signal for screen update
			m_TimeScrnBuf1[i] = m_TimeBuf1[bufpos];
			m_TimeScrnBuf2[i] = m_TimeBuf2[bufpos++];
			if(bufpos>=w)
				bufpos = 0;
		}
		emit NewTimeData();
	}
	m_PreviousSample = sample;
}

//////////////////////////////////////////////////////////////////////
// Called to display text in debug text box.
//////////////////////////////////////////////////////////////////////
void CTestBench::GotTxt(QString Str)
{
	ui->textEdit->append(Str);
}

/////////////////////////////////////////////////////////////////////
// Status Timer event handler
/////////////////////////////////////////////////////////////////////
void CTestBench::OnTimer()
{
	//update current sweep frequency text
	ui->labelFreq->setText(QString().setNum((int)m_SweepFrequency));
}

//////////////////////////////////////////////////////////////////////
// Called when screen size changes so must recalculate bitmaps
//////////////////////////////////////////////////////////////////////
void CTestBench::resizeEvent(QResizeEvent* )
{
	if(!ui->frameGraph->size().isValid())
		return;
	if( m_Rect != ui->frameGraph->geometry())
	{	//if changed, resize pixmaps to new frame size
		m_Rect = ui->frameGraph->geometry();
		m_OverlayPixmap = QPixmap(m_Rect.width(), m_Rect.height());
		m_OverlayPixmap.fill(Qt::black);
		m_2DPixmap = QPixmap(m_Rect.width(), m_Rect.height());
		m_2DPixmap.fill(Qt::black);
		Reset();
	}
	if(m_TimeDisplay)
		DrawTimeOverlay();
	else
		DrawFreqOverlay();
}

//////////////////////////////////////////////////////////////////////
// Called by QT when screen needs to be redrawn
//////////////////////////////////////////////////////////////////////
void CTestBench::paintEvent(QPaintEvent *)
{
	QPainter painter(this);
	painter.drawPixmap(m_Rect, m_2DPixmap);
	return;
}

//////////////////////////////////////////////////////////////////////
// Called to update time domain data for displaying on the screen
//////////////////////////////////////////////////////////////////////
void CTestBench::DrawTimePlot()
{
int i;
int w;
int h;
QPoint LineBuf[TB_MAX_SCREENSIZE];
	if(m_2DPixmap.isNull())
		return;

	if(m_NewDataIsCpx != m_CurrentDataIsCpx)
	{
		m_CurrentDataIsCpx = m_NewDataIsCpx;
	}
	//get/draw the 2D spectrum
	w = m_2DPixmap.width();
	h = m_2DPixmap.height();
	//first copy into 2Dbitmap the overlay bitmap.
	m_2DPixmap = m_OverlayPixmap.copy(0,0,w,h);
	QPainter painter2(&m_2DPixmap);

	//draw the time points
	int c = h/2;
	for(i=0; i<w; i++)
	{
		LineBuf[i].setX(i);
		LineBuf[i].setY(c - (2*c*m_TimeScrnBuf1[i])/m_VertRange );
	}
	painter2.setPen( Qt::green );
	painter2.drawPolyline(LineBuf,w);

	if(m_CurrentDataIsCpx)
	{
		for(i=0; i<w; i++)
		{
			LineBuf[i].setX(i);
			LineBuf[i].setY(c - (2*c*m_TimeScrnBuf2[i])/m_VertRange );
		}
		painter2.setPen( Qt::red );
		painter2.drawPolyline(LineBuf,w);
	}

	//trigger a new paintEvent
	update();
	if(	(TRIG_PSINGLE != m_TrigIndex) &&
		(TRIG_NSINGLE != m_TrigIndex) )
	{
		m_TrigState = TRIGSTATE_WAIT;
	}
}

//////////////////////////////////////////////////////////////////////
// Called to update spectrum data for displaying on the screen
//////////////////////////////////////////////////////////////////////
void CTestBench::DrawFftPlot()
{
int i;
int w;
int h;
qint32 fftbuf[TB_MAX_SCREENSIZE];
QPoint LineBuf[TB_MAX_SCREENSIZE];
	if(m_2DPixmap.isNull())
		return;
	if(m_NewDataIsCpx != m_CurrentDataIsCpx)
	{
		m_CurrentDataIsCpx = m_NewDataIsCpx;
		DrawFreqOverlay();
	}
	//get/draw the 2D spectrum
	w = m_2DPixmap.width();
	h = m_2DPixmap.height();
	//first copy into 2Dbitmap the overlay bitmap.
	m_2DPixmap = m_OverlayPixmap.copy(0,0,w,h);
	QPainter painter2(&m_2DPixmap);
	//get new scaled fft data
	if(m_CurrentDataIsCpx)
	{
		m_Fft.GetScreenIntegerFFTData( h, w,
							m_MaxdB,
							m_MindB,
							-m_Span/2,
							m_Span/2,
							fftbuf );
	}
	else
	{
		m_Fft.GetScreenIntegerFFTData( h, w,
							m_MaxdB,
							m_MindB,
							0.0,
							m_Span/2,
							fftbuf );

	}
	//draw the 2D spectrum
	for(i=0; i<w; i++)
	{
		LineBuf[i].setX(i);
		LineBuf[i].setY(fftbuf[i]);
		if(fftbuf[i] < m_FftPkBuf[i])
			m_FftPkBuf[i] = fftbuf[i];
	}
	painter2.setPen( Qt::green );
	painter2.drawPolyline(LineBuf,w);

	if(m_PeakOn)
	{
		for(i=0; i<w; i++)
		{
			LineBuf[i].setX(i);
			LineBuf[i].setY(m_FftPkBuf[i]);
		}
		painter2.setPen( Qt::red );
		painter2.drawPolyline(LineBuf,w);
	}
	//trigger a new paintEvent
	update();
}

//////////////////////////////////////////////////////////////////////
// Called to draw an overlay bitmap containing grid and text that
// does not need to be recreated every Time data update.
//////////////////////////////////////////////////////////////////////
void CTestBench::DrawTimeOverlay()
{
	if(m_OverlayPixmap.isNull())
		return;
int w = m_OverlayPixmap.width();
int h = m_OverlayPixmap.height();
int x,y;
float pixperdiv;
QRect rect;
	QPainter painter(&m_OverlayPixmap);
	painter.initFrom(this);

	m_OverlayPixmap.fill(Qt::black);

	//draw vertical grids
	pixperdiv = (float)w / (float)TB_HORZ_DIVS;
	y = h - h/TB_TIMEVERT_DIVS;
	painter.setPen(QPen(Qt::white, 0,Qt::DotLine));
	for( int i=1; i<TB_HORZ_DIVS; i++)
	{
		x = (int)( (float)i*pixperdiv );
		painter.drawLine(x, 0, x , y);
		painter.drawLine(x, h-5, x , h);
	}
	//create Font to use for scales
	QFont Font("Arial");
	Font.setPointSize(12);
	QFontMetrics metrics(Font);
	y = h/TB_TIMEVERT_DIVS;
	if(y<metrics.height())
		Font.setPixelSize(y);
	Font.setWeight(QFont::Normal);
	painter.setFont(Font);

	//draw time values
	painter.setPen(Qt::cyan);
	y = h - (h/TB_TIMEVERT_DIVS);
	double xval = 0;
	for( int i=0; i<=TB_HORZ_DIVS; i++)
	{
		if(0==i)
		{	//left justify the leftmost text
			x = (int)( (float)i*pixperdiv);
			rect.setRect(x ,y, (int)pixperdiv, h/TB_TIMEVERT_DIVS);
			painter.drawText(rect, Qt::AlignLeft|Qt::AlignVCenter, QString::number(xval));
		}
		else if(TB_HORZ_DIVS == i)
		{	//right justify the rightmost text
			x = (int)( (float)i*pixperdiv - pixperdiv);
			rect.setRect(x ,y, (int)pixperdiv, h/TB_TIMEVERT_DIVS);
			painter.drawText(rect, Qt::AlignRight|Qt::AlignVCenter,  QString::number(xval));
		}
		else
		{	//center justify the rest of the text
			x = (int)( (float)i*pixperdiv - pixperdiv/2);
			rect.setRect(x ,y, (int)pixperdiv, h/TB_TIMEVERT_DIVS);
			painter.drawText(rect, Qt::AlignHCenter|Qt::AlignVCenter,  QString::number(xval));
		}
		xval += (double)m_HorzSpan/(double)TB_HORZ_DIVS;
	}
	//draw horizontal grids
	pixperdiv = (float)h / (float)TB_TIMEVERT_DIVS;
	for( int i=1; i<TB_TIMEVERT_DIVS; i++)
	{
		y = (int)( (float)i*pixperdiv );
		if(i==TB_TIMEVERT_DIVS/2)
			painter.setPen(QPen(Qt::blue, 1,Qt::DotLine));
		else
			painter.setPen(QPen(Qt::white, 1,Qt::DotLine));
		painter.drawLine(0, y, w, y);
	}

	painter.setPen(QPen(Qt::yellow, 1,Qt::DotLine));
	int c = h/2;
	y = c - (2*c*m_TrigLevel)/m_VertRange;
	painter.drawLine(0, y, w, y);


	//draw amplitude values
	painter.setPen(Qt::darkYellow);
	Font.setWeight(QFont::Light);
	painter.setFont(Font);
	int yval = m_VertRange/2;
	for( int i=0; i<TB_TIMEVERT_DIVS-1; i++)
	{
		y = (int)( (float)i*pixperdiv );
		painter.drawStaticText(5, y-1, QString::number(yval));
		yval -= (m_VertRange/10);
	}
	//copy into 2Dbitmap the overlay bitmap.
	m_2DPixmap = m_OverlayPixmap.copy(0,0,w,h);
	//trigger a new paintEvent
	update();
}


//////////////////////////////////////////////////////////////////////
// Called to draw an overlay bitmap containing grid and text that
// does not need to be recreated every fft data update.
//////////////////////////////////////////////////////////////////////
void CTestBench::DrawFreqOverlay()
{
	if(m_OverlayPixmap.isNull())
		return;
int w = m_OverlayPixmap.width();
int h = m_OverlayPixmap.height();
int x,y;
float pixperdiv;
QRect rect;
	QPainter painter(&m_OverlayPixmap);
	painter.initFrom(this);

	m_OverlayPixmap.fill(Qt::black);

	//draw vertical grids
	pixperdiv = (float)w / (float)TB_HORZ_DIVS;
	y = h - h/TB_VERT_DIVS;
	for( int i=1; i<TB_HORZ_DIVS; i++)
	{
		x = (int)( (float)i*pixperdiv );
		if(i==TB_HORZ_DIVS/2)
			painter.setPen(QPen(Qt::red, 0,Qt::DotLine));
		else
			painter.setPen(QPen(Qt::white, 0,Qt::DotLine));
		painter.drawLine(x, 0, x , y);
		painter.drawLine(x, h-5, x , h);
	}
	//create Font to use for scales
	QFont Font("Arial");
	Font.setPointSize(12);
	QFontMetrics metrics(Font);
	y = h/TB_VERT_DIVS;
	if(y<metrics.height())
		Font.setPixelSize(y);
	Font.setWeight(QFont::Normal);
	painter.setFont(Font);

	//draw frequency values
	MakeFrequencyStrs();
	painter.setPen(Qt::cyan);
	y = h - (h/TB_VERT_DIVS);
	for( int i=0; i<=TB_HORZ_DIVS; i++)
	{
		if(0==i)
		{	//left justify the leftmost text
			x = (int)( (float)i*pixperdiv);
			rect.setRect(x ,y, (int)pixperdiv, h/TB_VERT_DIVS);
			painter.drawText(rect, Qt::AlignLeft|Qt::AlignVCenter, m_HDivText[i]);
		}
		else if(TB_HORZ_DIVS == i)
		{	//right justify the rightmost text
			x = (int)( (float)i*pixperdiv - pixperdiv);
			rect.setRect(x ,y, (int)pixperdiv, h/TB_VERT_DIVS);
			painter.drawText(rect, Qt::AlignRight|Qt::AlignVCenter, m_HDivText[i]);
		}
		else
		{	//center justify the rest of the text
			x = (int)( (float)i*pixperdiv - pixperdiv/2);
			rect.setRect(x ,y, (int)pixperdiv, h/TB_VERT_DIVS);
			painter.drawText(rect, Qt::AlignHCenter|Qt::AlignVCenter, m_HDivText[i]);
		}
	}

	//draw horizontal grids
	pixperdiv = (float)h / (float)TB_VERT_DIVS;
	painter.setPen(QPen(Qt::white, 1,Qt::DotLine));
	for( int i=1; i<TB_VERT_DIVS; i++)
	{
		y = (int)( (float)i*pixperdiv );
		painter.drawLine(0, y, w, y);
	}

	//draw amplitude values
	painter.setPen(Qt::darkYellow);
	Font.setWeight(QFont::Light);
	painter.setFont(Font);
	int dB = m_MaxdB;
	for( int i=0; i<TB_VERT_DIVS-1; i++)
	{
		y = (int)( (float)i*pixperdiv );
		painter.drawStaticText(5, y-1, QString::number(dB)+" dB");
		dB -= m_dBStepSize;
	}
	m_MindB = m_MaxdB - (TB_VERT_DIVS)*m_dBStepSize;

	//copy into 2Dbitmap the overlay bitmap.
	m_2DPixmap = m_OverlayPixmap.copy(0,0,w,h);
	//trigger a new paintEvent
	update();
}

//////////////////////////////////////////////////////////////////////
// Helper function Called to create all the frequency division text
//strings based on start frequency, span frequency, frequency units.
//Places in QString array m_HDivText
//Keeps all strings the same fractional length
//////////////////////////////////////////////////////////////////////
void CTestBench::MakeFrequencyStrs()
{
qint64 FreqPerDiv;
qint64 StartFreq;
float freq;
int i,j;
int numfractdigits = (int)log10((double)m_FreqUnits);
	if(m_CurrentDataIsCpx)
	{
		FreqPerDiv = m_Span/TB_HORZ_DIVS;
		StartFreq = m_CenterFreq - m_Span/2;
	}
	else
	{
		FreqPerDiv = m_Span/(2*TB_HORZ_DIVS);
		StartFreq = 0;
	}
	if(1 == m_FreqUnits)
	{	//if units is Hz then just output integer freq
		for(int i=0; i<=TB_HORZ_DIVS; i++)
		{
			freq = (float)StartFreq/(float)m_FreqUnits;
			m_HDivText[i].setNum((int)freq);
			StartFreq += FreqPerDiv;
		}
		return;
	}
	//here if is fractional frequency values
	//so create max sized text based on frequency units
	for(int i=0; i<=TB_HORZ_DIVS; i++)
	{
		freq = (float)StartFreq/(float)m_FreqUnits;
		m_HDivText[i].setNum(freq,'f', numfractdigits);
		StartFreq += FreqPerDiv;
	}
	//now find the division text with the longest non-zero digit
	//to the right of the decimal point.
	int max = 0;
	for(i=0; i<=TB_HORZ_DIVS; i++)
	{
		int dp = m_HDivText[i].indexOf('.');
		int l = m_HDivText[i].length()-1;
		for(j=l; j>dp; j--)
		{
			if(m_HDivText[i][j] != '0')
				break;
		}
		if( (j-dp) > max)
			max = j-dp;
	}
	//truncate all strings to maximum fractional length
	StartFreq = m_CenterFreq - m_Span/2;
	for( i=0; i<=TB_HORZ_DIVS; i++)
	{
		freq = (float)StartFreq/(float)m_FreqUnits;
		m_HDivText[i].setNum(freq,'f', max);
		StartFreq += FreqPerDiv;
	}
}
