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
  void setUIntVal(std::uint8_t &setValue, int newValue);
  void setIntVal(int &setValue, int newValue);
  void setFloatVal(float &setValue, float newValue);
  void setBoolVal(bool &setValue, bool newValue);
  void onUploadButtonClick();
  void onSaveButtonClick();
};
#endif // MAINWINDOW_H
