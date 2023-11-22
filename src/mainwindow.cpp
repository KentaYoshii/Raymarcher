#include "mainwindow.h"
#include "settings.h"

#include <QCheckBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QTabWidget>
#include <QVBoxLayout>
#include <iostream>

MainWindow::MainWindow() {
  setWindowTitle("Final Project");

  // horizontal
  QHBoxLayout *hLayout = new QHBoxLayout();
  // vertical
  QVBoxLayout *vLayout = new QVBoxLayout();
  vLayout->setAlignment(Qt::AlignTop);
  hLayout->addLayout(vLayout);
  setLayout(hLayout);

  setupScreen();
  resize(1024, 768);

  // makes the Screen into a scroll area
  QScrollArea *scrollArea = new QScrollArea();
  scrollArea->setWidget(m_screen);
  scrollArea->setWidgetResizable(true);
  hLayout->addWidget(scrollArea, 1);

  // set layout
  QWidget *screen = new QWidget();
  QVBoxLayout *screenLayout = new QVBoxLayout();
  screenLayout->setAlignment(Qt::AlignTop);
  screen->setLayout(screenLayout);

  vLayout->addWidget(screen);

  // upload scene file
  addPushButton(screenLayout, "Upload Scenefile",
                &MainWindow::onUploadButtonClick);

  // save screen as image
  addPushButton(screenLayout, "Save Image", &MainWindow::onSaveButtonClick);
}

/**
 * @brief Sets up Screen
 */
void MainWindow::setupScreen() {
  m_screen = new Screen();
  m_screen->init();
}

// ------ FUNCTIONS FOR ADDING UI COMPONENTS ------

void MainWindow::addHeading(QBoxLayout *layout, QString text) {
  QFont font;
  font.setPointSize(16);
  font.setBold(true);

  QLabel *label = new QLabel(text);
  label->setFont(font);
  layout->addWidget(label);
}

void MainWindow::addLabel(QBoxLayout *layout, QString text) {
  layout->addWidget(new QLabel(text));
}

void MainWindow::addRadioButton(QBoxLayout *layout, QString text, bool value,
                                auto function) {
  QRadioButton *button = new QRadioButton(text);
  button->setChecked(value);
  layout->addWidget(button);
  connect(button, &QRadioButton::clicked, this, function);
}

void MainWindow::addSpinBox(QBoxLayout *layout, QString text, int min, int max,
                            int step, int val, auto function) {
  QSpinBox *box = new QSpinBox();
  box->setMinimum(min);
  box->setMaximum(max);
  box->setSingleStep(step);
  box->setValue(val);
  QHBoxLayout *subLayout = new QHBoxLayout();
  addLabel(subLayout, text);
  subLayout->addWidget(box);
  layout->addLayout(subLayout);
  connect(box, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
          this, function);
}

void MainWindow::addDoubleSpinBox(QBoxLayout *layout, QString text, double min,
                                  double max, double step, double val,
                                  int decimal, auto function) {
  QDoubleSpinBox *box = new QDoubleSpinBox();
  box->setMinimum(min);
  box->setMaximum(max);
  box->setSingleStep(step);
  box->setValue(val);
  box->setDecimals(decimal);
  QHBoxLayout *subLayout = new QHBoxLayout();
  addLabel(subLayout, text);
  subLayout->addWidget(box);
  layout->addLayout(subLayout);
  connect(box,
          static_cast<void (QDoubleSpinBox::*)(double)>(
              &QDoubleSpinBox::valueChanged),
          this, function);
}

void MainWindow::addPushButton(QBoxLayout *layout, QString text,
                               auto function) {
  QPushButton *button = new QPushButton(text);
  layout->addWidget(button);
  connect(button, &QPushButton::clicked, this, function);
}

void MainWindow::addCheckBox(QBoxLayout *layout, QString text, bool val,
                             auto function) {
  QCheckBox *box = new QCheckBox(text);
  box->setChecked(val);
  layout->addWidget(box);
  connect(box, &QCheckBox::clicked, this, function);
}

// ------ FUNCTIONS FOR UPDATING SETTINGS ------
void MainWindow::setUIntVal(std::uint8_t &setValue, int newValue) {
  setValue = newValue;
  m_screen->settingsChanged();
}

void MainWindow::setIntVal(int &setValue, int newValue) {
  setValue = newValue;
  m_screen->settingsChanged();
}

void MainWindow::setFloatVal(float &setValue, float newValue) {
  setValue = newValue;
  m_screen->settingsChanged();
}

void MainWindow::setBoolVal(bool &setValue, bool newValue) {
  setValue = newValue;
  m_screen->settingsChanged();
}

// ------ PUSH BUTTON FUNCTIONS ------
void MainWindow::onUploadButtonClick() {
  // Get abs path of scene file
  QString configFilePath = QFileDialog::getOpenFileName(
      this, tr("Upload File"),
      QDir::currentPath().append(QDir::separator()).append("scenefiles"),
      tr("Scene Files (*.json)"));
  if (configFilePath.isNull()) {
    std::cout << "Failed to load null scenefile." << std::endl;
    return;
  }

  settings.sceneFilePath = configFilePath.toStdString();

  std::cout << "Loaded scenefile: \"" << configFilePath.toStdString() << "\"."
            << std::endl;
}

void MainWindow::onSaveButtonClick() {
  // Get new image path selected by user
  QString file =
      QFileDialog::getSaveFileName(this, tr("Save Image"), QDir::currentPath(),
                                   tr("Image Files (*.png *.jpg *.jpeg)"));
  if (file.isEmpty()) {
    return;
  }

  // Save image
  m_screen->saveImageToFile(file);
}
