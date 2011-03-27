//////////////////////////////////////////////////////////////////////
// ipeditwidget.h: interface for the CIPEditWidget class.
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
/////////////////////////////////////////////////////////////////////
#ifndef IPEDITWIDGET_H
#define IPEDITWIDGET_H

#include <QWidget>
#include "ui_ipeditframe.h"

class CIPEditWidget : public QFrame
{
    Q_OBJECT
public:
    explicit CIPEditWidget(QWidget *parent = 0);
	~CIPEditWidget();

	void SetIP(quint32 val);
	void SetIP(quint8 b3, quint8 b2, quint8 b1, quint8 b0);
	bool GetIP(quint8& b3, quint8& b2, quint8& b1, quint8& b0);
	bool GetIP(quint32& ip);
	quint32 GetIP32(){GetControlValues(); return m_ip;}
	quint8 GetIP0(){GetControlValues(); return m_ip0;}
	quint8 GetIP1(){GetControlValues(); return m_ip1;}
	quint8 GetIP2(){GetControlValues(); return m_ip2;}
	quint8 GetIP3(){GetControlValues(); return m_ip3;}

signals:

public slots:

private slots:

protected:

private:
	Ui::FrameIPdit ui;
	void UpdateControls();
	void GetControlValues();
	QIntValidator* m_pIPAddressValidator;
	bool m_Dirty;
	quint32 m_ip;
	quint8 m_ip0;
	quint8 m_ip1;
	quint8 m_ip2;
	quint8 m_ip3;
};

#endif // IPEDITWIDGET_H
