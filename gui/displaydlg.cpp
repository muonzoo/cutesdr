/////////////////////////////////////////////////////////////////////
// displaydlg.cpp: implementation of the CDisplayDlg class.
//
//	This class implements a dialog to setup the display parameters
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
#include "displaydlg.h"
#include "ui_displaydlg.h"
#include <QDebug>

CDisplayDlg::CDisplayDlg(QWidget *parent) :
    QDialog(parent),
	ui(new Ui::CDisplayDlg)
{
    ui->setupUi(this);
	m_FftAve = 0;
	m_FftSize = 4096;
	m_MaxDisplayRate = 10;
	m_Percent2DScreen = 50;
}

CDisplayDlg::~CDisplayDlg()
{
    delete ui;
}

//Fill in initial data
void CDisplayDlg::InitDlg()
{
	ui->fftSizecomboBox->addItem("2048 pts", 2048);
	ui->fftSizecomboBox->addItem("4096 pts", 4096);
	ui->fftSizecomboBox->addItem("8192 pts", 8192);
	ui->fftSizecomboBox->addItem("16384 pts", 16384);
	ui->fftSizecomboBox->addItem("32768 pts", 32768);

	int index = ui->fftSizecomboBox->findData(m_FftSize);
	if(index<0)
	{
		index = 1;
		m_FftSize = 4096;
	}
	ui->fftSizecomboBox->setCurrentIndex(index);
	ui->fftAvespinBox->setValue(m_FftAve);
	ui->ClickResolutionspinBox->setValue(m_ClickResolution);
	ui->MaxDisplayRatespinBox->setValue(m_MaxDisplayRate);
	ui->Screen2DSizespinBox->setValue(m_Percent2DScreen);
	ui->checkBoxTestBench->setChecked(m_UseTestBench);
	m_NeedToStop = false;
}

//Called when OK button pressed so get all the dialog data
void CDisplayDlg::accept()
{
	m_FftSize = ui->fftSizecomboBox->itemData(ui->fftSizecomboBox->currentIndex()).toInt();
	int tmpsz = ui->Screen2DSizespinBox->value();
	if( tmpsz!=m_Percent2DScreen)
	{
		m_NeedToStop = true;
		m_Percent2DScreen = tmpsz;
	}
	m_FftAve = ui->fftAvespinBox->value();
	m_ClickResolution = ui->ClickResolutionspinBox->value();
	m_MaxDisplayRate = ui->MaxDisplayRatespinBox->value();
	m_UseTestBench = ui->checkBoxTestBench->isChecked();
	QDialog::accept();	//need to call base class
}

