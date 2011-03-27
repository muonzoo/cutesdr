/////////////////////////////////////////////////////////////////////
// demodsetupdlg.cpp: implementation of the CDemodSetupDlg class.
//
//	This class implements a dialog to setup the demodulation parameters
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
#include "gui/demodsetupdlg.h"
#include "ui_demodsetupdlg.h"
#include "gui/mainwindow.h"
#include <QDebug>

CDemodSetupDlg::CDemodSetupDlg(QWidget *parent) :
	QDialog(parent),
   ui(new Ui::CDemodSetupDlg)
{
	ui->setupUi(this);
	m_pDemodInfo = NULL;
	m_DemodMode = DEMOD_AM;
	connect(ui->frameSlope, SIGNAL(sliderValChanged(int)), this, SLOT(OnAgcSlope(int)));
	connect(ui->frameThresh, SIGNAL(sliderValChanged(int)), this, SLOT(OnAgcThresh(int)));
	connect(ui->frameDecay, SIGNAL(sliderValChanged(int)), this, SLOT(OnAgcDecay(int)));
	ui->frameSlope->SetName("Slope");
	ui->frameSlope->SetSuffix(" dB");
	ui->frameSlope->setRange(0, 10);
	ui->frameSlope->setSingleStep(1);
	ui->frameSlope->setPageStep(1);
	ui->frameSlope->setTickInterval(1);

	ui->frameThresh->SetName("Knee");
	ui->frameThresh->SetSuffix(" dB");
	ui->frameThresh->setRange(-120, -30);
	ui->frameThresh->setSingleStep(5);
	ui->frameThresh->setPageStep(5);
	ui->frameThresh->setTickInterval(10);

	ui->frameDecay->SetName("Decay");
	ui->frameDecay->SetSuffix(" mS");
	ui->frameDecay->setRange(20, 2000);
	ui->frameDecay->setSingleStep(10);
	ui->frameDecay->setPageStep(100);
	ui->frameDecay->setTickInterval(200);
}

CDemodSetupDlg::~CDemodSetupDlg()
{
    delete ui;
}

void CDemodSetupDlg::OnAgcOn(bool On)
{
	if(m_pDemodInfo)
	{
		m_pDemodInfo->AgcOn = On;
		UpdateDemodInfo();
		((MainWindow*)this->parent())->SetupDemod(m_DemodMode);
	}
}

void CDemodSetupDlg::OnHangOn(bool On)
{
	if(m_pDemodInfo)
	{
		m_pDemodInfo->AgcHangOn = On;
		UpdateDemodInfo();
		((MainWindow*)this->parent())->SetupDemod(m_DemodMode);
	}
}

//Fill in initial data
void CDemodSetupDlg::InitDlg()
{
	switch(m_DemodMode)
	{
	case DEMOD_AM:
		ui->AMradioButton->setChecked(TRUE);
		break;
	case DEMOD_SAM:
		ui->SAMradioButton->setChecked(TRUE);
		break;
	case DEMOD_FM:
		ui->FMradioButton->setChecked(TRUE);
		break;
	case DEMOD_USB:
		ui->USBradioButton->setChecked(TRUE);
		break;
	case DEMOD_LSB:
		ui->LSBradioButton->setChecked(TRUE);
		break;
	case DEMOD_CWU:
		ui->CWUradioButton->setChecked(TRUE);
		break;
	case DEMOD_CWL:
		ui->CWLradioButton->setChecked(TRUE);
		break;
	}
	m_pDemodInfo = &(((MainWindow*)this->parent())->m_DemodSettings[m_DemodMode]);
	UpdateDemodInfo();
}


void CDemodSetupDlg::UpdateDemodInfo()
{
int tmp;
	ui->checkBoxAgcOn->setChecked(m_pDemodInfo->AgcOn);
	ui->checkBoxHang->setChecked(m_pDemodInfo->AgcHangOn);
	if(m_pDemodInfo->AgcOn)
	{
		ui->frameThresh->SetName("Knee");
		tmp = m_pDemodInfo->AgcThresh;//save since range change triggers a value change for some reason
		ui->frameThresh->setRange(-120, -20);
		ui->frameThresh->SetValue(tmp);
	}
	else
	{
		ui->frameThresh->SetName("Gain");
		tmp = m_pDemodInfo->AgcManualGain;//save since range change triggers a value change for some reason
		ui->frameThresh->setRange(0, 100);
		ui->frameThresh->SetValue(tmp);
	}
	if(m_pDemodInfo->AgcHangOn)
	{
		ui->frameDecay->SetName("Hang");
	}
	else
	{
		ui->frameDecay->SetName("Decay");
	}
	ui->spinBoxOffset->setValue(m_pDemodInfo->Offset);
	ui->horizontalSliderSquelch->setValue(m_pDemodInfo->SquelchValue);
	ui->frameSlope->SetValue(m_pDemodInfo->AgcSlope);
	ui->frameDecay->SetValue(m_pDemodInfo->AgcDecay);
}

void CDemodSetupDlg::OnOffsetChanged(int Offset)
{
	if(m_pDemodInfo)
	{
		m_pDemodInfo->Offset = Offset;
		((MainWindow*)this->parent())->SetupDemod(m_DemodMode);
	}
}

void CDemodSetupDlg::OnSquelchChanged(int SquelchValue)
{
	if(m_pDemodInfo)
	{
		m_pDemodInfo->SquelchValue = SquelchValue;
		((MainWindow*)this->parent())->SetupDemod(m_DemodMode);
	}
}


//Called when new mode is pressed so set the dialog data
void CDemodSetupDlg::ModeChanged()
{
	if(ui->AMradioButton->isChecked())
		m_DemodMode = DEMOD_AM;
	else if(ui->SAMradioButton->isChecked())
		m_DemodMode = DEMOD_SAM;
	else if(ui->FMradioButton->isChecked())
		m_DemodMode = DEMOD_FM;
	else if(ui->USBradioButton->isChecked())
		m_DemodMode = DEMOD_USB;
	else if(ui->LSBradioButton->isChecked())
		m_DemodMode = DEMOD_LSB;
	else if(ui->CWUradioButton->isChecked())
		m_DemodMode = DEMOD_CWU;
	else if(ui->CWLradioButton->isChecked())
		m_DemodMode = DEMOD_CWL;
	((MainWindow*)this->parent())->SetupDemod(m_DemodMode);
	m_pDemodInfo = &(((MainWindow*)this->parent())->m_DemodSettings[m_DemodMode]);
	UpdateDemodInfo();
}

void CDemodSetupDlg::OnAgcSlope(int val)
{
	if(m_pDemodInfo)
	{
		m_pDemodInfo->AgcSlope = val;
		((MainWindow*)this->parent())->SetupDemod(m_DemodMode);
	}
}

void CDemodSetupDlg::OnAgcThresh(int val)
{
	if(m_pDemodInfo)
	{
		if(m_pDemodInfo->AgcOn)
			m_pDemodInfo->AgcThresh = val;
		else
			m_pDemodInfo->AgcManualGain = val;
		((MainWindow*)this->parent())->SetupDemod(m_DemodMode);
	}
}

void CDemodSetupDlg::OnAgcDecay(int val)
{
	if(m_pDemodInfo)
	{
		m_pDemodInfo->AgcDecay = val;
		((MainWindow*)this->parent())->SetupDemod(m_DemodMode);
	}
}
