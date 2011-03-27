//////////////////////////////////////////////////////////////////////
// aboutdlg.h: interface for the CAboutDlg class.
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
/////////////////////////////////////////////////////////////////////
#ifndef ABOUTDLG_H
#define ABOUTDLG_H

#include <QDialog>

namespace Ui {
    class CAboutDlg;
}

class CAboutDlg : public QDialog
{
    Q_OBJECT

public:
	explicit CAboutDlg(QWidget *parent=0, QString Revision="");
    ~CAboutDlg();

private:
	QString m_Revision;
	Ui::CAboutDlg *ui;
	QString m_Str;
};

#endif // ABOUTDLG_H
