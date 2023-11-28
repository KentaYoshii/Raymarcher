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
  near_label->setText("Near Plane");
  QLabel *far_label = new QLabel(); // Far plane label
  far_label->setText("Far Plane");
  QLabel *renderoption_label = new QLabel();
  renderoption_label->setText("Render Options");
  renderoption_label->setFont(font);
  QLabel *postproc_option_label = new QLabel();
  postproc_option_label->setText("Post-processing Options:");
  postproc_option_label->setFont(font);
  QLabel *screen_color_label = new QLabel();
  screen_color_label->setText("Select Disp Option");
  screen_color_label->setFont(font);
  QLabel *skybox_label = new QLabel();
  skybox_label->setText("Select SkyBox");
  skybox_label->setFont(font);
  QLabel *eps_label = new QLabel();
  eps_label->setText("Exposure");

  softShadow = new QCheckBox();
  softShadow->setText(QStringLiteral("Soft Shadow"));
  softShadow->setChecked(false);

  reflection = new QCheckBox();
  reflection->setText(QStringLiteral("Reflection"));
  reflection->setChecked(false);

  refraction = new QCheckBox();
  refraction->setText(QStringLiteral("Refraction"));
  refraction->setChecked(false);

  ambientOcculusion = new QCheckBox();
  ambientOcculusion->setText(QStringLiteral("Ambient Occulusion"));
  ambientOcculusion->setChecked(false);

  fxaa = new QCheckBox();
  fxaa->setText(QStringLiteral("FXAA"));
  fxaa->setChecked(false);

  skyboxOption = new QComboBox();
  skyboxOption->addItem("None");
  skyboxOption->addItem("Beach");
  skyboxOption->addItem("Night Sky");
  skyboxOption->addItem("Island");
  skyboxOption->setCurrentIndex(0);

  lightOption = new QComboBox();
  lightOption->addItem("None");
  lightOption->addItem("Gamma Correct");
  lightOption->addItem("HDR");
  lightOption->addItem("Bloom");
  lightOption->setCurrentIndex(0);

  // Create file uploader for scene file
  uploadFile = new QPushButton();
  uploadFile->setText(QStringLiteral("Upload Scene File"));

  saveImage = new QPushButton();
  saveImage->setText(QStringLiteral("Save image"));

  nearBox = new QDoubleSpinBox();
  nearBox->setMinimum(0.01f);
  nearBox->setMaximum(10.f);
  nearBox->setSingleStep(0.5f);
  nearBox->setValue(0.1f);

  farBox = new QDoubleSpinBox();
  farBox->setMinimum(10.f);
  farBox->setMaximum(100.f);
  farBox->setSingleStep(0.5f);
  farBox->setValue(100.f);

  epsilonBox = new QDoubleSpinBox();
  epsilonBox->setMinimum(0.1f);
  epsilonBox->setMaximum(5.f);
  epsilonBox->setSingleStep(0.1f);
  epsilonBox->setValue(1.0f);

  QGroupBox *nearLayout = new QGroupBox(); // horizonal near slider alignment
  QHBoxLayout *lnear = new QHBoxLayout();
  QGroupBox *farLayout = new QGroupBox(); // horizonal far slider alignment
  QHBoxLayout *lfar = new QHBoxLayout();
  QHBoxLayout *epsLayout = new QHBoxLayout();

  // Adds the slider and number box to the parameter layouts
  lnear->addWidget(near_label);
  lnear->addWidget(nearBox);
  nearLayout->setLayout(lnear);

  lfar->addWidget(far_label);
  lfar->addWidget(farBox);
  farLayout->setLayout(lfar);

  epsLayout->addWidget(eps_label);
  epsLayout->addWidget(epsilonBox);

  vLayout->addWidget(uploadFile);
  vLayout->addWidget(saveImage);
  vLayout->addWidget(camera_label);
  vLayout->addWidget(nearLayout);
  vLayout->addWidget(farLayout);
  vLayout->addWidget(renderoption_label);
  vLayout->addWidget(softShadow);
  vLayout->addWidget(reflection);
  vLayout->addWidget(refraction);
  vLayout->addWidget(ambientOcculusion);
  vLayout->addWidget(skybox_label);
  vLayout->addWidget(skyboxOption);
  vLayout->addWidget(postproc_option_label);
  vLayout->addWidget(fxaa);
  vLayout->addWidget(screen_color_label);
  vLayout->addWidget(lightOption);
  vLayout->addLayout(epsLayout);

  connectUIElements();

  // Set default values for near and far planes
  onValChangeNearBox(0.1f);
  onValChangeFarBox(100.f);
  onSkyBox(0);
  onDispOption(0);
  onEpsilon(1.f);
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
  connectSoftShadow();
  connectReflection();
  connectRefraction();
  connectAmbientOcculusion();
  connectFXAA();
  connectSkyBox();
  connectDispOption();
  connectEpsilon();
}

void MainWindow::connectUploadFile() {
  connect(uploadFile, &QPushButton::clicked, this, &MainWindow::onUploadFile);
}

void MainWindow::connectSaveImage() {
  connect(saveImage, &QPushButton::clicked, this, &MainWindow::onSaveImage);
}

void MainWindow::connectNear() {
  connect(nearBox,
          static_cast<void (QDoubleSpinBox::*)(double)>(
              &QDoubleSpinBox::valueChanged),
          this, &MainWindow::onValChangeNearBox);
}

void MainWindow::connectFar() {
  connect(farBox,
          static_cast<void (QDoubleSpinBox::*)(double)>(
              &QDoubleSpinBox::valueChanged),
          this, &MainWindow::onValChangeFarBox);
}

void MainWindow::connectSoftShadow() {
  connect(softShadow, &QCheckBox::clicked, this, &MainWindow::onSoftShadow);
}

void MainWindow::connectReflection() {
  connect(reflection, &QCheckBox::clicked, this, &MainWindow::onReflection);
}

void MainWindow::connectRefraction() {
  connect(refraction, &QCheckBox::clicked, this, &MainWindow::onRefraction);
}

void MainWindow::connectAmbientOcculusion() {
  connect(ambientOcculusion, &QCheckBox::clicked, this,
          &MainWindow::onAmbientOcculusion);
}

void MainWindow::connectFXAA() {
  connect(fxaa, &QCheckBox::clicked, this, &MainWindow::onFXAA);
}

void MainWindow::connectSkyBox() {
  connect(skyboxOption, &QComboBox::currentIndexChanged, this,
          &MainWindow::onSkyBox);
}

void MainWindow::connectDispOption() {
  connect(lightOption, &QComboBox::currentIndexChanged, this,
          &MainWindow::onDispOption);
}

void MainWindow::connectEpsilon() {
  connect(epsilonBox,
          static_cast<void (QDoubleSpinBox::*)(double)>(
              &QDoubleSpinBox::valueChanged),
          this, &MainWindow::onEpsilon);
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

void MainWindow::onValChangeNearBox(double newValue) {
  // nearBox->setValue(newValue);
  settings.nearPlane = nearBox->value();
  realtime->settingsChanged();
}

void MainWindow::onValChangeFarBox(double newValue) {
  // farBox->setValue(newValue);
  settings.farPlane = farBox->value();
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

void MainWindow::onRefraction() {
  settings.enableRefraction = !settings.enableRefraction;
  realtime->settingsChanged();
}

void MainWindow::onAmbientOcculusion() {
  settings.enableAmbientOcculusion = !settings.enableAmbientOcculusion;
  realtime->settingsChanged();
}

void MainWindow::onFXAA() {
  settings.enableFXAA = !settings.enableFXAA;
  realtime->settingsChanged();
}

void MainWindow::onSkyBox(int idx) {
  settings.idxSkyBox = idx;
  realtime->settingsChanged();
}

void MainWindow::onDispOption(int idx) {
  settings.enableGammaCorrection = false;
  settings.enableHDR = false;
  settings.enableBloom = false;
  switch (idx) {
  case 0:
    break;
  case 1:
    settings.enableGammaCorrection = true;
    break;
  case 2:
    settings.enableHDR = true;
    break;
  case 3:
    settings.enableBloom = true;
    break;
  }
  realtime->settingsChanged();
}

void MainWindow::onEpsilon(double newValue) {
  settings.exposure = newValue;
  realtime->settingsChanged();
}
