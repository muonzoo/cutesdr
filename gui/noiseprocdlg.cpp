/////////////////////////////////////////////////////////////////////
// noiseprocdlg.cpp: implementation of the CNoiseProcDlg class.
//
//	This class implements a dialog to setup the noise processing parameters
//
// History:
//	2011-02-05  Initial creation MSW
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
#include "gui/noiseprocdlg.h"
#include "gui/mainwindow.h"
#include "ui_noiseprocdlg.h"
#include <QDebug>

CNoiseProcDlg::CNoiseProcDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CNoiseProcDlg)
{
    ui->setupUi(this);
	m_pNoiseProcSettings = NULL;
	connect(ui->frameNBWidth, SIGNAL(sliderValChanged(int)), this, SLOT(OnNBWidth(int)));
	connect(ui->frameNBThresh, SIGNAL(sliderValChanged(int)), this, SLOT(OnNBThresh(int)));
	ui->frameNBThresh->SetName("Thresh");
	ui->frameNBThresh->SetSuffix(" ");
	ui->frameNBThresh->setRange(0, 99);
	ui->frameNBThresh->setSingleStep(1);
	ui->frameNBThresh->setPageStep(1);
	ui->frameNBThresh->setTickInterval(10);

	ui->frameNBWidth->SetName("Width");
	ui->frameNBWidth->SetSuffix(" uSec");
	ui->frameNBWidth->setRange(10, 300);
	ui->frameNBWidth->setSingleStep(10);
	ui->frameNBWidth->setPageStep(10);
	ui->frameNBWidth->setTickInterval(30);

}

CNoiseProcDlg::~CNoiseProcDlg()
{
    delete ui;
}

//Fill in initial data
void CNoiseProcDlg::InitDlg(tNoiseProcdInfo* pNoiseProcSettings)
{
	if(NULL == pNoiseProcSettings)
		return;
	m_pNoiseProcSettings = pNoiseProcSettings;
	if(m_pNoiseProcSettings->NBOn)
		ui->checkBoxNBOn->setChecked(true);
	else
		ui->checkBoxNBOn->setChecked(false);
	ui->frameNBThresh->SetValue(m_pNoiseProcSettings->NBThreshold);
	ui->frameNBWidth->SetValue(m_pNoiseProcSettings->NBWidth);
}

void CNoiseProcDlg::OnNBOn(bool On)
{
	if(m_pNoiseProcSettings)
	{
		m_pNoiseProcSettings->NBOn = On;
		((MainWindow*)this->parent())->SetupNoiseProc();
	}
}

void CNoiseProcDlg::OnNBThresh(int val)
{
	if(m_pNoiseProcSettings)
	{
		m_pNoiseProcSettings->NBThreshold = val;
		((MainWindow*)this->parent())->SetupNoiseProc();
	}
}

void CNoiseProcDlg::OnNBWidth(int val)
{
	if(m_pNoiseProcSettings)
	{
		m_pNoiseProcSettings->NBWidth = val;
		((MainWindow*)this->parent())->SetupNoiseProc();
	}
}


