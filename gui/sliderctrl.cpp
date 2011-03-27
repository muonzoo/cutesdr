/////////////////////////////////////////////////////////////////////
// sliderctrl.cpp: implementation of the CSliderCtrl class.
//
//	This class implements a slider control widget
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
#include "gui/sliderctrl.h"
#include <QDebug>


CSliderCtrl::CSliderCtrl(QWidget *parent) :
	QFrame(parent)
{
	ui.setupUi(this);
	ui.labelName->setText("");
	ui.labelVal->setText("");
	m_Prefix = "";
	m_Suffix = "";
	ui.horizontalSlider->setValue(0);
}

CSliderCtrl::~CSliderCtrl()
{
}

void CSliderCtrl::valueChanged(int val)
{
int newval = val;
int delta = ui.horizontalSlider->singleStep();
	//keep value on singleStep increments
	val = val - val%delta;
	if(newval != val)
		ui.horizontalSlider->setValue(val);
	emit sliderValChanged(val);
	DisplayValue(val);
}

void CSliderCtrl::DisplayValue(int val)
{
	QString str;
	ui.labelVal->setText(m_Prefix + str.setNum(val) + m_Suffix);
}


