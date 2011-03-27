//////////////////////////////////////////////////////////////////////
// sliderctrl.h: interface for the CSliderCtrl class.
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
/////////////////////////////////////////////////////////////////////
#ifndef SLIDERCTRL_H
#define SLIDERCTRL_H

#include <QWidget>
#include "ui_sliderctrl.h"

class CSliderCtrl : public QFrame
{
    Q_OBJECT
public:
	explicit CSliderCtrl(QWidget *parent = 0);
	~CSliderCtrl();
	//map slider functions to control
	void SetValue(int val){ui.horizontalSlider->setValue(val);}
	void setRange(int min, int max){ui.horizontalSlider->setRange(min,max);}
	void setTickInterval(int ti){ui.horizontalSlider->setTickInterval(ti);}
	void setPageStep(int ps){ui.horizontalSlider->setPageStep(ps);}
	void setSingleStep(int ss){ui.horizontalSlider->setSingleStep(ss);}

	void SetName(QString Name){ui.labelName->setText(Name);}
	void SetPrefix(QString Prefix){m_Prefix = Prefix;}
	void SetSuffix(QString Suffix){m_Suffix = Suffix;}

signals:
	void sliderValChanged(int);

public slots:

private slots:
	void valueChanged(int);

protected:

private:
	void DisplayValue(int val);
	Ui::sliderctrl ui;
	QString m_Suffix;
	QString m_Prefix;
};

#endif // SLIDERCTRL_H
