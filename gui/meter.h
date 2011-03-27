//////////////////////////////////////////////////////////////////////
// meter.h: interface for the CMeter class.
//
// History:
//	2010-12-28  Initial creation MSW
//	2011-03-27  Initial release
/////////////////////////////////////////////////////////////////////
#ifndef METER_H
#define METER_H

#include <QtGui>
#include <QFrame>
#include <QImage>
#include "interface/sdrinterface.h"


class CMeter : public QFrame
{
    Q_OBJECT
public:
	explicit CMeter(QWidget *parent = 0);
	~CMeter();

	QSize minimumSizeHint() const;
	QSize sizeHint() const;

	void draw();		//call to draw new fft data onto screen plot
	void UpdateOverlay(){DrawOverlay();}

signals:

public slots:
	void SetdBmLevel(double dbm);

protected:
		//re-implemented widget event handlers
	void paintEvent(QPaintEvent *event);
	void resizeEvent(QResizeEvent* event);

private:
	void DrawOverlay();
	QPixmap m_2DPixmap;
	QPixmap m_OverlayPixmap;
	QSize m_Size;
	QString m_Str;
	int m_Slevel;
	int m_dBm;
	CSdrInterface* m_pSdrInterface;
};

#endif // METER_H
