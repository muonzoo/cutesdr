/////////////////////////////////////////////////////////////////////
// sounddlg.cpp: implementation of the CSoundDlg class.
//
//	This class implements a dialog to select a soundcard.
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
#include "gui/sounddlg.h"
#include <QAudioInput>
#include <QAudioOutput>

/////////////////////////////////////////////////////////////////////
// Constructor/Destructor
/////////////////////////////////////////////////////////////////////
CSoundDlg::CSoundDlg(QWidget *parent) :	QDialog(parent)
{
QList<QAudioDeviceInfo> InDevices;
QList<QAudioDeviceInfo> OutDevices;
	ui.setupUi(this);	//setup the dialog form
	QAudioDeviceInfo deviceInfo;
	InDevices = deviceInfo.availableDevices(QAudio::AudioInput);
	OutDevices = deviceInfo.availableDevices(QAudio::AudioOutput);
	foreach (const QAudioDeviceInfo &deviceInfo, InDevices)
		ui.comboBoxSndIn->addItem(deviceInfo.deviceName(), qVariantFromValue(deviceInfo));
	foreach (const QAudioDeviceInfo &deviceInfo, OutDevices)
		ui.comboBoxSndOut->addItem(deviceInfo.deviceName(), qVariantFromValue(deviceInfo));
}

CSoundDlg::~CSoundDlg()
{

}

