//////////////////////////////////////////////////////////////////////
// sdrsetupdlg.h: interface for the CSdrSetupDlg class.
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
/////////////////////////////////////////////////////////////////////
#ifndef SDRSETUPDLG_H
#define SDRSETUPDLG_H

#include <QDialog>
#include "interface/sdrinterface.h"

namespace Ui {
    class CSdrSetupDlg;
}

class CSdrSetupDlg : public QDialog
{
    Q_OBJECT

public:
	explicit CSdrSetupDlg(QWidget *parent, CSdrInterface*  pSdrInterface);
    ~CSdrSetupDlg();

	void InitDlg();
	qint32 m_RfGain;
	qint32 m_BandwidthIndex;


public slots:
	void accept();
	void RfGainChanged();

private:
	CSdrInterface* m_pSdrInterface;
	Ui::CSdrSetupDlg *ui;
	QString m_Str1;
	QString m_Str2;
	qint32 m_RadioType;
	double m_SampleRate;
};

#endif // SDRSETUPDLG_H
