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

  // brush selection
  addHeading(screenLayout, "Brush");
  addRadioButton(screenLayout, "Constant", settings.brushType == BRUSH_CONSTANT,
                 [this] { setBrushType(BRUSH_CONSTANT); });
  addRadioButton(screenLayout, "Linear", settings.brushType == BRUSH_LINEAR,
                 [this] { setBrushType(BRUSH_LINEAR); });
  addRadioButton(screenLayout, "Quadratic",
                 settings.brushType == BRUSH_QUADRATIC,
                 [this] { setBrushType(BRUSH_QUADRATIC); });
  addRadioButton(screenLayout, "Smudge", settings.brushType == BRUSH_SMUDGE,
                 [this] { setBrushType(BRUSH_SMUDGE); });

  // brush parameters
  addSpinBox(screenLayout, "red", 0, 255, 1, settings.brushColor.r,
             [this](int value) { setUIntVal(settings.brushColor.r, value); });
  addSpinBox(screenLayout, "green", 0, 255, 1, settings.brushColor.g,
             [this](int value) { setUIntVal(settings.brushColor.g, value); });
  addSpinBox(screenLayout, "blue", 0, 255, 1, settings.brushColor.b,
             [this](int value) { setUIntVal(settings.brushColor.b, value); });
  addSpinBox(screenLayout, "alpha", 0, 255, 1, settings.brushColor.a,
             [this](int value) { setUIntVal(settings.brushColor.a, value); });
  addSpinBox(screenLayout, "radius", 0, 100, 1, settings.brushRadius,
             [this](int value) { setIntVal(settings.brushRadius, value); });

  // extra credit brushes
  addHeading(screenLayout, "Extra Credit Brushes");
  addRadioButton(screenLayout, "Spray", settings.brushType == BRUSH_SPRAY,
                 [this] { setBrushType(BRUSH_SPRAY); });
  addSpinBox(screenLayout, "density", 0, 100, 1, settings.brushDensity,
             [this](int value) { setIntVal(settings.brushDensity, value); });
  addRadioButton(screenLayout, "Speed", settings.brushType == BRUSH_SPEED,
                 [this] { setBrushType(BRUSH_SPEED); });
  addRadioButton(screenLayout, "Fill", settings.brushType == BRUSH_FILL,
                 [this] { setBrushType(BRUSH_FILL); });
  addRadioButton(screenLayout, "Custom", settings.brushType == BRUSH_CUSTOM,
                 [this] { setBrushType(BRUSH_CUSTOM); });
  addCheckBox(
      screenLayout, "Fix alpha blending", settings.fixAlphaBlending,
      [this](bool value) { setBoolVal(settings.fixAlphaBlending, value); });

  // clearing canvas
  addPushButton(screenLayout, "Clear Screen", &MainWindow::onClearButtonClick);

  // save canvas as image
  addPushButton(screenLayout, "Save Image", &MainWindow::onSaveButtonClick);
}

/**
 * @brief Sets up Canvas2D
 */
void MainWindow::setupScreen() {
  m_screen = new Screen();
  m_screen->init();

  if (!settings.imagePath.isEmpty()) {
    m_screen->loadImageFromFile(settings.imagePath);
  }
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

void MainWindow::setBrushType(int type) {
  settings.brushType = type;
  m_screen->settingsChanged();
}

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

void MainWindow::onClearButtonClick() {
  m_screen->resize(m_screen->parentWidget()->size().width(),
                   m_screen->parentWidget()->size().height());
  m_screen->clearScreen();
}

void MainWindow::onRevertButtonClick() {
  m_screen->loadImageFromFile(settings.imagePath);
}

void MainWindow::onUploadButtonClick() {
  // Get new image path selected by user
  QString file =
      QFileDialog::getOpenFileName(this, tr("Open Image"), QDir::homePath(),
                                   tr("Image Files (*.png *.jpg *.jpeg)"));
  if (file.isEmpty()) {
    return;
  }
  settings.imagePath = file;

  // Display new image
  m_screen->loadImageFromFile(settings.imagePath);

  m_screen->settingsChanged();
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
