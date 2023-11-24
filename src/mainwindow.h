#pragma once

#include "realtime.h"
#include "utils/aspectratiowidget/aspectratiowidget.hpp"
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QMainWindow>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>

class MainWindow : public QWidget {
  Q_OBJECT

public:
  void initialize();
  void finish();

private:
  void connectUIElements();
  void connectNear();
  void connectFar();
  void connectGammaCorrect();
  void connectSoftShadow();
  void connectUploadFile();
  void connectSaveImage();

  Realtime *realtime;
  AspectRatioWidget *aspectRatioWidget;

  QPushButton *uploadFile;
  QPushButton *saveImage;
  QSlider *nearSlider;
  QSlider *farSlider;
  QDoubleSpinBox *nearBox;
  QDoubleSpinBox *farBox;

  QCheckBox *gammaCorrection;
  QCheckBox *softShadow;

private slots:
  void onUploadFile();
  void onSaveImage();
  void onValChangeNearSlider(int newValue);
  void onValChangeFarSlider(int newValue);
  void onValChangeNearBox(double newValue);
  void onValChangeFarBox(double newValue);
  void onGammaCorrect();
  void onSoftShadow();
};
