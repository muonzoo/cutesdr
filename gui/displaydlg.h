//////////////////////////////////////////////////////////////////////
// displaydlg.h: interface for the CDisplaydlg class.
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
/////////////////////////////////////////////////////////////////////
#ifndef DISPLAYDLG_H
#define DISPLAYDLG_H

#include <QDialog>
#include "gui/mainwindow.h"
#include "ui_displaydlg.h"
#include "gui/testbench.h"

namespace Ui {
	class CDisplayDlg;
}

class CDisplayDlg : public QDialog
{
    Q_OBJECT

public:
	explicit CDisplayDlg(QWidget *parent = 0);
	~CDisplayDlg();

	void InitDlg();
	int m_FftAve;
	int m_FftSize;
	int m_ClickResolution;
	int m_MaxDisplayRate;
	int m_Percent2DScreen;
	bool m_NeedToStop;
	bool m_UseTestBench;

public slots:
	void accept();

private:
	Ui::CDisplayDlg *ui;

};

#endif // DISPLAYDLG_H
