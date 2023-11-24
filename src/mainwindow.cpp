#include "mainwindow.h"
#include "settings.h"

#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSettings>
#include <QVBoxLayout>
#include <iostream>

void MainWindow::initialize() {
  realtime = new Realtime;
  aspectRatioWidget = new AspectRatioWidget(this);
  aspectRatioWidget->setAspectWidget(realtime, 3.f / 4.f);
  QHBoxLayout *hLayout = new QHBoxLayout;   // horizontal alignment
  QVBoxLayout *vLayout = new QVBoxLayout(); // vertical alignment
  vLayout->setAlignment(Qt::AlignTop);
  hLayout->addLayout(vLayout);
  hLayout->addWidget(aspectRatioWidget, 1);
  this->setLayout(hLayout);

  // Create labels in sidebox
  QFont font;
  font.setPointSize(12);
  font.setBold(true);
  QLabel *camera_label = new QLabel(); // Camera label
  camera_label->setText("Camera");
  camera_label->setFont(font);
  QLabel *near_label = new QLabel(); // Near plane label
  near_label->setText("Near Plane:");
  QLabel *far_label = new QLabel(); // Far plane label
  far_label->setText("Far Plane:");

  gammaCorrection = new QCheckBox();
  gammaCorrection->setText(QStringLiteral("Gamma Correction"));
  gammaCorrection->setChecked(false);

  softShadow = new QCheckBox();
  softShadow->setText(QStringLiteral("Soft Shadow"));
  softShadow->setChecked(false);

  reflection = new QCheckBox();
  reflection->setText(QStringLiteral("Reflection"));
  reflection->setChecked(false);

  // Create file uploader for scene file
  uploadFile = new QPushButton();
  uploadFile->setText(QStringLiteral("Upload Scene File"));

  saveImage = new QPushButton();
  saveImage->setText(QStringLiteral("Save image"));

  // Creates the boxes containing the camera sliders and number boxes
  QGroupBox *nearLayout = new QGroupBox(); // horizonal near slider alignment
  QHBoxLayout *lnear = new QHBoxLayout();
  QGroupBox *farLayout = new QGroupBox(); // horizonal far slider alignment
  QHBoxLayout *lfar = new QHBoxLayout();

  // Create slider controls to control near/far planes
  nearSlider = new QSlider(Qt::Orientation::Horizontal); // Near plane slider
  nearSlider->setTickInterval(1);
  nearSlider->setMinimum(1);
  nearSlider->setMaximum(1000);
  nearSlider->setValue(10);

  nearBox = new QDoubleSpinBox();
  nearBox->setMinimum(0.01f);
  nearBox->setMaximum(10.f);
  nearBox->setSingleStep(0.1f);
  nearBox->setValue(0.1f);

  farSlider = new QSlider(Qt::Orientation::Horizontal); // Far plane slider
  farSlider->setTickInterval(1);
  farSlider->setMinimum(1000);
  farSlider->setMaximum(10000);
  farSlider->setValue(10000);

  farBox = new QDoubleSpinBox();
  farBox->setMinimum(10.f);
  farBox->setMaximum(100.f);
  farBox->setSingleStep(0.1f);
  farBox->setValue(100.f);

  // Adds the slider and number box to the parameter layouts
  lnear->addWidget(nearSlider);
  lnear->addWidget(nearBox);
  nearLayout->setLayout(lnear);

  lfar->addWidget(farSlider);
  lfar->addWidget(farBox);
  farLayout->setLayout(lfar);

  vLayout->addWidget(uploadFile);
  vLayout->addWidget(saveImage);
  vLayout->addWidget(camera_label);
  vLayout->addWidget(near_label);
  vLayout->addWidget(nearLayout);
  vLayout->addWidget(far_label);
  vLayout->addWidget(farLayout);
  vLayout->addWidget(gammaCorrection);
  vLayout->addWidget(softShadow);
  vLayout->addWidget(reflection);

  connectUIElements();

  // Set default values for near and far planes
  onValChangeNearBox(0.1f);
  onValChangeFarBox(10.f);
}

void MainWindow::finish() {
  realtime->finish();
  delete (realtime);
}

void MainWindow::connectUIElements() {
  connectUploadFile();
  connectSaveImage();
  connectNear();
  connectFar();
  connectGammaCorrect();
  connectSoftShadow();
  connectReflection();
}

void MainWindow::connectUploadFile() {
  connect(uploadFile, &QPushButton::clicked, this, &MainWindow::onUploadFile);
}

void MainWindow::connectSaveImage() {
  connect(saveImage, &QPushButton::clicked, this, &MainWindow::onSaveImage);
}

void MainWindow::connectNear() {
  connect(nearSlider, &QSlider::valueChanged, this,
          &MainWindow::onValChangeNearSlider);
  connect(nearBox,
          static_cast<void (QDoubleSpinBox::*)(double)>(
              &QDoubleSpinBox::valueChanged),
          this, &MainWindow::onValChangeNearBox);
}

void MainWindow::connectFar() {
  connect(farSlider, &QSlider::valueChanged, this,
          &MainWindow::onValChangeFarSlider);
  connect(farBox,
          static_cast<void (QDoubleSpinBox::*)(double)>(
              &QDoubleSpinBox::valueChanged),
          this, &MainWindow::onValChangeFarBox);
}

void MainWindow::connectGammaCorrect() {
  connect(gammaCorrection, &QCheckBox::clicked, this,
          &MainWindow::onGammaCorrect);
}

void MainWindow::connectSoftShadow() {
  connect(softShadow, &QCheckBox::clicked, this, &MainWindow::onSoftShadow);
}

void MainWindow::connectReflection() {
  connect(reflection, &QCheckBox::clicked, this, &MainWindow::onReflection);
}

void MainWindow::onUploadFile() {
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

  realtime->sceneChanged();
}

void MainWindow::onSaveImage() {
  if (settings.sceneFilePath.empty()) {
    std::cout << "No scene file loaded." << std::endl;
    return;
  }
  std::string sceneName = settings.sceneFilePath.substr(
      0, settings.sceneFilePath.find_last_of("."));
  sceneName = sceneName.substr(sceneName.find_last_of("/") + 1);
  QString filePath = QFileDialog::getSaveFileName(
      this, tr("Save Image"),
      QDir::currentPath().append(QDir::separator()).append("output"),
      tr("Image Files (*.png)"));
  std::cout << "Saving image to: \"" << filePath.toStdString() << "\"."
            << std::endl;
  realtime->saveViewportImage(filePath.toStdString());
}

void MainWindow::onValChangeNearSlider(int newValue) {
  // nearSlider->setValue(newValue);
  nearBox->setValue(newValue / 100.f);
  settings.nearPlane = nearBox->value();
  realtime->settingsChanged();
}

void MainWindow::onValChangeFarSlider(int newValue) {
  // farSlider->setValue(newValue);
  farBox->setValue(newValue / 100.f);
  settings.farPlane = farBox->value();
  realtime->settingsChanged();
}

void MainWindow::onValChangeNearBox(double newValue) {
  nearSlider->setValue(int(newValue * 100.f));
  // nearBox->setValue(newValue);
  settings.nearPlane = nearBox->value();
  realtime->settingsChanged();
}

void MainWindow::onValChangeFarBox(double newValue) {
  farSlider->setValue(int(newValue * 100.f));
  // farBox->setValue(newValue);
  settings.farPlane = farBox->value();
  realtime->settingsChanged();
}

void MainWindow::onGammaCorrect() {
  settings.enableGammaCorrection = !settings.enableGammaCorrection;
  realtime->settingsChanged();
}

void MainWindow::onSoftShadow() {
  settings.enableSoftShadow = !settings.enableSoftShadow;
  realtime->settingsChanged();
}

void MainWindow::onReflection() {
  settings.enableReflection = !settings.enableReflection;
  realtime->settingsChanged();
}
