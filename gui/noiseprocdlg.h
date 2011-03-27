//////////////////////////////////////////////////////////////////////
// noiseprocdlg.h: interface for the CNoiseProcDlg class.
//
// History:
//	2011-02-6  Initial creation MSW
//	2011-03-27  Initial release
/////////////////////////////////////////////////////////////////////
#ifndef NOISEPROCDLG_H
#define NOISEPROCDLG_H

#include <QDialog>
#include "dsp/noiseproc.h"


namespace Ui {
    class CNoiseProcDlg;
}

class CNoiseProcDlg : public QDialog
{
    Q_OBJECT

public:
    explicit CNoiseProcDlg(QWidget *parent = 0);
    ~CNoiseProcDlg();

	void InitDlg(tNoiseProcdInfo* pNoiseProcSettings);

private slots:
	void OnNBThresh(int);
	void OnNBWidth(int);
	void OnNBOn(bool);


private:
    Ui::CNoiseProcDlg *ui;
	tNoiseProcdInfo* m_pNoiseProcSettings;
};

#endif // NOISEPROCDLG_H
