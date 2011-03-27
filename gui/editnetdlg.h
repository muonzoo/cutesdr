//////////////////////////////////////////////////////////////////////
// editnetdlg.h: interface for the CEditNetDlg class.
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
/////////////////////////////////////////////////////////////////////
#ifndef EDITNETDLG_H
#define EDITNETDLG_H

#include <QDialog>
#include "gui/mainwindow.h"
#include "ui_editnetdlg.h"

class CEditNetDlg : public QDialog
{
    Q_OBJECT
public:
	explicit CEditNetDlg(QWidget *parent = 0);
	~CEditNetDlg();

	void InitDlg();
	bool eventFilter(QObject* o, QEvent* e);

	QHostAddress m_IPAdr;
	quint32 m_Port;
	QString m_ActiveDevice;
	bool m_DirtyFlag;

signals:

public slots:
	void accept();
	void FindSdrs();

private:
	Ui::EditNetDlg ui;
	QIntValidator* m_pPortAddressValidator;
};

#endif // EDITNETDLG_H
