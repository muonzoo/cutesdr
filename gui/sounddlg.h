//////////////////////////////////////////////////////////////////////
// sounddlg.h: interface for the CSoundDlg class.
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
/////////////////////////////////////////////////////////////////////
#ifndef SOUNDDLG_H
#define SOUNDDLG_H

#include <QDialog>
#include "ui_sounddlg.h"


class CSoundDlg : public QDialog
{
	Q_OBJECT
public:
	CSoundDlg(QWidget *parent=0);
	~CSoundDlg();
	void SetInputIndex(int x){ui.comboBoxSndIn->setCurrentIndex(x); }
	void SetOutputIndex(int x){ui.comboBoxSndOut->setCurrentIndex(x); }
	int GetInputIndex(){return ui.comboBoxSndIn->currentIndex(); }
	int GetOutputIndex(){return ui.comboBoxSndOut->currentIndex(); }
	void SetStereo(bool Stereo){ui.checkBoxStereo->setChecked(Stereo);}
	bool GetStereo(){return ui.checkBoxStereo->checkState();}

public slots:

signals:

private slots:

private:
	Ui::DialogSndCard ui;

};

#endif // SOUNDDLG_H
