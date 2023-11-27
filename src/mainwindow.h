#pragma once

#include "realtime.h"
#include "utils/aspectratiowidget/aspectratiowidget.hpp"
#include <QCheckBox>
#include <QComboBox>
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
  void connectSoftShadow();
  void connectReflection();
  void connectRefraction();
  void connectAmbientOcculusion();
  void connectFXAA();
  void connectSkyBox();
  void connectDispOption();
  void connectUploadFile();
  void connectSaveImage();

  Realtime *realtime;
  AspectRatioWidget *aspectRatioWidget;

  QPushButton *uploadFile;
  QPushButton *saveImage;
  QDoubleSpinBox *nearBox;
  QDoubleSpinBox *farBox;

  QCheckBox *softShadow;
  QCheckBox *reflection;
  QCheckBox *refraction;
  QCheckBox *ambientOcculusion;
  QCheckBox *fxaa;
  QComboBox *skyboxOption;
  QComboBox *lightOption;

private slots:
  void onUploadFile();
  void onSaveImage();
  void onValChangeNearBox(double newValue);
  void onValChangeFarBox(double newValue);
  void onSoftShadow();
  void onReflection();
  void onRefraction();
  void onAmbientOcculusion();
  void onFXAA();
  void onSkyBox(int idx);
  void onDispOption(int idx);
};
