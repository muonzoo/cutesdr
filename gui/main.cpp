#include <QtGui/QApplication>
#include "gui/mainwindow.h"

int main(int argc, char *argv[])
{
//	QApplication::setGraphicsSystem("native");
	QApplication::setGraphicsSystem("raster");
//	QApplication::setGraphicsSystem("opengl");
	QApplication a(argc, argv);
	MainWindow w;
	w.show();
	return a.exec();
}
