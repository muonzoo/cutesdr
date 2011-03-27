/////////////////////////////////////////////////////////////////////
// ipeditwidget.cpp: implementation of the CIPEditWidget class.
//
//	This class implements a widget to set/modify an IPV4 address
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
#include "gui/ipeditwidget.h"
#include <QDebug>


CIPEditWidget::CIPEditWidget(QWidget *parent) :
	QFrame(parent)
{
	ui.setupUi(this);
	m_pIPAddressValidator =  new QIntValidator(0, 255, this);

	ui.lineEdit_B0->setValidator(m_pIPAddressValidator);
	ui.lineEdit_B1->setValidator(m_pIPAddressValidator);
	ui.lineEdit_B2->setValidator(m_pIPAddressValidator);
	ui.lineEdit_B3->setValidator(m_pIPAddressValidator);
	m_Dirty = FALSE;
	m_ip = 0;
}

CIPEditWidget::~CIPEditWidget()
{
	delete m_pIPAddressValidator;
}


void CIPEditWidget::SetIP(quint32 val)
{//initialize control with IP address data in val
	m_ip = val;
	m_ip0 = val&0xFF;
	m_ip1 = (val>>8)&0xFF;
	m_ip2 = (val>>16)&0xFF;
	m_ip3 = (val>>24)&0xFF;
	UpdateControls();
}

void CIPEditWidget::SetIP(quint8 b3, quint8 b2, quint8 b1, quint8 b0)
{
	m_ip0 = b0;
	m_ip1 = b1;
	m_ip2 = b2;
	m_ip3 = b3;
	m_ip = m_ip3; m_ip<<=8;
	m_ip += m_ip2; m_ip<<=8;
	m_ip += m_ip1; m_ip<<=8;
	m_ip += m_ip0;
	UpdateControls();
}

bool CIPEditWidget::GetIP(quint8& b3, quint8& b2, quint8& b1, quint8& b0)
{
	GetControlValues();
	b3 = m_ip3;
	b2 = m_ip2;
	b1 = m_ip1;
	b0 = m_ip0;
	return m_Dirty;
}

bool CIPEditWidget::GetIP(quint32& ip)
{
	GetControlValues();
	ip = m_ip;
	return m_Dirty;
}

void CIPEditWidget::UpdateControls()
{
	ui.lineEdit_B3->setText( QString().number( m_ip3 ));
	ui.lineEdit_B2->setText( QString().number( m_ip2 ));
	ui.lineEdit_B1->setText( QString().number( m_ip1 ));
	ui.lineEdit_B0->setText( QString().number( m_ip0 ));
}

void CIPEditWidget::GetControlValues()
{
	if(ui.lineEdit_B3->isModified() && ui.lineEdit_B3->hasAcceptableInput() )
	{
		m_Dirty = TRUE;
		m_ip3 = ui.lineEdit_B3->text().toUInt();
	}
	if(ui.lineEdit_B2->isModified() && ui.lineEdit_B2->hasAcceptableInput() )
	{
		m_Dirty = TRUE;
		m_ip2 = ui.lineEdit_B2->text().toUInt();
	}
	if(ui.lineEdit_B1->isModified() && ui.lineEdit_B1->hasAcceptableInput() )
	{
		m_Dirty = TRUE;
		m_ip1 = ui.lineEdit_B1->text().toUInt();
	}
	if(ui.lineEdit_B0->isModified() && ui.lineEdit_B0->hasAcceptableInput() )
	{
		m_Dirty = TRUE;
		m_ip0 = ui.lineEdit_B0->text().toUInt();
	}
	m_ip = m_ip3; m_ip<<=8;
	m_ip += m_ip2; m_ip<<=8;
	m_ip += m_ip1; m_ip<<=8;
	m_ip += m_ip0;
}
