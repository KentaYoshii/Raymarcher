#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QBoxLayout>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>

#include "screen.h"

class MainWindow : public QWidget {
  Q_OBJECT

public:
  MainWindow();

private:
  void setupScreen();
  Screen *m_screen;
  // Per-Pixel Filter
  QCheckBox *invertFilter;
  QCheckBox *grayScaleFilter;
  QCheckBox *darkenFilter;
  // Kernel Filter
  QCheckBox *sharpenFilter;
  QCheckBox *boxFilter;
  QCheckBox *sobelFilter;

  QPushButton *uploadFile;
  QPushButton *saveImage;
  QSlider *p1Slider;
  QSlider *p2Slider;
  QSpinBox *p1Box;
  QSpinBox *p2Box;
  QSlider *nearSlider;
  QSlider *farSlider;
  QDoubleSpinBox *nearBox;
  QDoubleSpinBox *farBox;

  // Extra Credit:
  QCheckBox *ec1;
  QCheckBox *ec2;
  QCheckBox *ec3;
  QCheckBox *ec4;
  QCheckBox *ec5;

  void addHeading(QBoxLayout *layout, QString text);
  void addLabel(QBoxLayout *layout, QString text);
  void addRadioButton(QBoxLayout *layout, QString text, bool value,
                      auto function);
  void addSpinBox(QBoxLayout *layout, QString text, int min, int max, int step,
                  int val, auto function);
  void addDoubleSpinBox(QBoxLayout *layout, QString text, double min,
                        double max, double step, double val, int decimal,
                        auto function);
  void addPushButton(QBoxLayout *layout, QString text, auto function);
  void addCheckBox(QBoxLayout *layout, QString text, bool value, auto function);

private slots:
  void setBrushType(int type);

  void setUIntVal(std::uint8_t &setValue, int newValue);
  void setIntVal(int &setValue, int newValue);
  void setFloatVal(float &setValue, float newValue);
  void setBoolVal(bool &setValue, bool newValue);

  void onClearButtonClick();
  void onRevertButtonClick();
  void onUploadButtonClick();
  void onSaveButtonClick();
};
#endif // MAINWINDOW_H
