#include "mainwindow.h"

#include <QApplication>
#include <QScreen>
#include <QSettings>
#include <iostream>

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);

  QCoreApplication::setApplicationName("Raymarching");
  QCoreApplication::setOrganizationName("PhongTotal");
  QCoreApplication::setApplicationVersion(QT_VERSION_STR);

  QSurfaceFormat fmt;
  fmt.setVersion(4, 1);
  fmt.setProfile(QSurfaceFormat::CoreProfile);
  QSurfaceFormat::setDefaultFormat(fmt);

  MainWindow w;
  w.initialize();
  w.resize(800, 600);
  w.show();

  int return_val = a.exec();
  w.finish();
  return return_val;
}
