//////////////////////////////////////////////////////////////////////
// demodsetupdlg.h: interface for the CDemodSetupDlg class.
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
/////////////////////////////////////////////////////////////////////
#ifndef DEMODSETUPDLG_H
#define DEMODSETUPDLG_H

#include <QDialog>
#include "dsp/demodulator.h"
#include "gui/sliderctrl.h"


namespace Ui {
    class CDemodSetupDlg;
}

class CDemodSetupDlg : public QDialog
{
    Q_OBJECT

public:
	explicit CDemodSetupDlg(QWidget *parent);
    ~CDemodSetupDlg();

	void InitDlg();
	int m_DemodMode;

private slots:
	void ModeChanged();
	void OnOffsetChanged(int Offset);
	void OnSquelchChanged(int SquelchValue);
	void OnAgcSlope(int);
	void OnAgcThresh(int);
	void OnAgcDecay(int);
	void OnAgcOn(bool);
	void OnHangOn(bool);


private:
	void UpdateDemodInfo();
	Ui::CDemodSetupDlg *ui;
	tDemodInfo* m_pDemodInfo;
};

#endif // DEMODSETUPDLG_H
