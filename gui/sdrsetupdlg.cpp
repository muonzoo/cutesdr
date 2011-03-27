/////////////////////////////////////////////////////////////////////
// sdrsetupdlg.cpp: implementation of the CSdrSetupDlg class.
//
//	This class implements a dialog to setup SDR parameters.
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
#include "gui/sdrsetupdlg.h"
#include "ui_sdrsetupdlg.h"
#include "gui/mainwindow.h"
#include <QDebug>

CSdrSetupDlg::CSdrSetupDlg(QWidget *parent, CSdrInterface* pSdrInterface) :
    QDialog(parent),
	m_pSdrInterface(pSdrInterface),
    ui(new Ui::CSdrSetupDlg)
{
    ui->setupUi(this);
	m_BandwidthIndex = 0;
	m_RfGain = pSdrInterface->GetSdrRfGain();
	m_RadioType = pSdrInterface->GetRadioType();
	m_SampleRate = pSdrInterface->GetSdrSampleRate();

}

CSdrSetupDlg::~CSdrSetupDlg()
{
    delete ui;
}

//Fill in initial data
void CSdrSetupDlg::InitDlg()
{
	m_Str1 = QString().number( m_pSdrInterface->GetMaxBWFromIndex(0)/1000 )+ " KHz  ";
	m_Str2 = QString().number(m_pSdrInterface->GetSampleRateFromIndex(0)/1000.0)+ " Ksps";
	ui->radioButtonRate0->setText(m_Str1 + m_Str2 );
	m_Str1 = QString().number( m_pSdrInterface->GetMaxBWFromIndex(1)/1000 )+ " KHz  ";
	m_Str2 = QString().number(m_pSdrInterface->GetSampleRateFromIndex(1)/1000.0)+ " Ksps";
	ui->radioButtonRate1->setText(m_Str1 + m_Str2 );
	m_Str1 = QString().number( m_pSdrInterface->GetMaxBWFromIndex(2)/1000 )+ " KHz  ";
	m_Str2 = QString().number(m_pSdrInterface->GetSampleRateFromIndex(2)/1000.0)+ " Ksps";
	ui->radioButtonRate2->setText(m_Str1 + m_Str2 );
	m_Str1 = QString().number( m_pSdrInterface->GetMaxBWFromIndex(3)/1000 )+ " KHz  ";
	m_Str2 = QString().number(m_pSdrInterface->GetSampleRateFromIndex(3)/1000.0)+ " Ksps";
	ui->radioButtonRate3->setText(m_Str1 + m_Str2 );

	if(0==m_RfGain)
		ui->radioButtonAttn0->setChecked(TRUE);
	else if(-10==m_RfGain)
		ui->radioButtonAttn10->setChecked(TRUE);
	else if(-20==m_RfGain)
		ui->radioButtonAttn20->setChecked(TRUE);
	else if(-30==m_RfGain)
		ui->radioButtonAttn30->setChecked(TRUE);

	if(0 == m_BandwidthIndex)
		ui->radioButtonRate0->setChecked(TRUE);
	else if(1 == m_BandwidthIndex)
		ui->radioButtonRate1->setChecked(TRUE);
	else if(2 == m_BandwidthIndex)
		ui->radioButtonRate2->setChecked(TRUE);
	else if(3 == m_BandwidthIndex)
		ui->radioButtonRate3->setChecked(TRUE);

	ui->labelRadioType->setText(m_pSdrInterface->m_DeviceName);
}

void CSdrSetupDlg::RfGainChanged()
{
	if(ui->radioButtonAttn0->isChecked())
		m_RfGain = 0;
	else if(ui->radioButtonAttn10->isChecked())
		m_RfGain = -10;
	else if(ui->radioButtonAttn20->isChecked())
		m_RfGain = -20;
	else if(ui->radioButtonAttn30->isChecked())
		m_RfGain = -30;
	m_pSdrInterface->SetSdrRfGain(m_RfGain);
}


//Called when OK button pressed so get all the dialog data
void CSdrSetupDlg::accept()
{
	if(ui->radioButtonRate0->isChecked())
		m_BandwidthIndex = 0;
	else if(ui->radioButtonRate1->isChecked())
		m_BandwidthIndex = 1;
	else if(ui->radioButtonRate2->isChecked())
		m_BandwidthIndex = 2;
	else if(ui->radioButtonRate3->isChecked())
		m_BandwidthIndex = 3;

	QDialog::accept();
}


