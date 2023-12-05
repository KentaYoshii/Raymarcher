#include "mainwindow.h"
#include "settings.h"

#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSettings>
#include <QVBoxLayout>
#include <filesystem>
#include <iostream>
#include <random>

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
  // Camera label
  QLabel *camera_label = new QLabel();
  camera_label->setText("Camera");
  camera_label->setFont(font);
  // Near plane label
  QLabel *near_label = new QLabel();
  near_label->setText("Near Plane");
  // Far plane label
  QLabel *far_label = new QLabel();
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
  QLabel *fractal_label = new QLabel();
  fractal_label->setText("Select Fractals");
  fractal_label->setFont(font);
  QLabel *power_label = new QLabel();
  power_label->setText("Power");
  QLabel *proc_label = new QLabel();
  proc_label->setText("Procedural Options");
  proc_label->setFont(font);
  QLabel *oct_label = new QLabel();
  oct_label->setText("Number of Octaves");
  QLabel *th_label = new QLabel();
  th_label->setText("Terrain Height");
  QLabel *ts_label = new QLabel();
  ts_label->setText("Terrain Scale");

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

  fractalOption = new QComboBox();
  fractalOption->addItem("None");
  fractalOption->addItem("Mandelbrot");
  fractalOption->addItem("Mandelbulb");
  fractalOption->addItem("Menger Sponge");
  fractalOption->addItem("Sierpinski Triangle");
  fractalOption->setCurrentIndex(0);

  // Create file uploader for scene file
  uploadFile = new QPushButton();
  uploadFile->setText(QStringLiteral("Upload Scene File"));

  saveImage = new QPushButton();
  saveImage->setText(QStringLiteral("Save image"));

  juliaSeed = new QPushButton();
  juliaSeed->setText(QStringLiteral("Generate Julia Seed"));

  nearBox = new QDoubleSpinBox();
  nearBox->setMinimum(0.01f);
  nearBox->setMaximum(10.f);
  nearBox->setSingleStep(0.5f);
  nearBox->setValue(0.1f);

  farBox = new QDoubleSpinBox();
  farBox->setMinimum(10.f);
  farBox->setMaximum(300.f);
  farBox->setSingleStep(1.f);
  farBox->setValue(100.f);

  epsilonBox = new QDoubleSpinBox();
  epsilonBox->setMinimum(0.01f);
  epsilonBox->setMaximum(5.f);
  epsilonBox->setSingleStep(0.1f);
  epsilonBox->setValue(1.0f);

  powerBox = new QDoubleSpinBox();
  powerBox->setMinimum(1.f);
  powerBox->setMaximum(30.f);
  powerBox->setSingleStep(0.1f);
  powerBox->setValue(8.0f);

  octaveBox = new QDoubleSpinBox();
  octaveBox->setMinimum(1);
  octaveBox->setMaximum(15);
  octaveBox->setSingleStep(1);
  octaveBox->setValue(8);

  terrainH = new QDoubleSpinBox();
  terrainH->setMinimum(0);
  terrainH->setMaximum(10);
  terrainH->setSingleStep(0.5);
  terrainH->setValue(10);

  terrainS = new QDoubleSpinBox();
  terrainS->setMinimum(0);
  terrainS->setMaximum(15);
  terrainS->setSingleStep(0.25);
  terrainS->setValue(2.75);

  QGroupBox *nearLayout = new QGroupBox(); // horizonal near slider alignment
  QHBoxLayout *lnear = new QHBoxLayout();
  QGroupBox *farLayout = new QGroupBox(); // horizonal far slider alignment
  QHBoxLayout *lfar = new QHBoxLayout();
  QHBoxLayout *epsLayout = new QHBoxLayout();
  QHBoxLayout *powerLayout = new QHBoxLayout();
  QHBoxLayout *octLayout = new QHBoxLayout();
  QHBoxLayout *terrainHL = new QHBoxLayout();
  QHBoxLayout *terrainSL = new QHBoxLayout();

  // Adds the slider and number box to the parameter layouts
  lnear->addWidget(near_label);
  lnear->addWidget(nearBox);
  nearLayout->setLayout(lnear);

  lfar->addWidget(far_label);
  lfar->addWidget(farBox);
  farLayout->setLayout(lfar);

  epsLayout->addWidget(eps_label);
  epsLayout->addWidget(epsilonBox);

  powerLayout->addWidget(power_label);
  powerLayout->addWidget(powerBox);

  octLayout->addWidget(oct_label);
  octLayout->addWidget(octaveBox);

  terrainHL->addWidget(th_label);
  terrainHL->addWidget(terrainH);

  terrainSL->addWidget(ts_label);
  terrainSL->addWidget(terrainS);

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
  vLayout->addWidget(fractal_label);
  vLayout->addWidget(fractalOption);
  vLayout->addLayout(powerLayout);
  vLayout->addWidget(juliaSeed);
  vLayout->addWidget(proc_label);
  vLayout->addLayout(terrainHL);
  vLayout->addLayout(terrainSL);
  vLayout->addLayout(octLayout);

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
  connectFractal();
  connectPower();
  connectJuliaSeed();
  connectOctave();
  connectTerrainH();
  connectTerrainS();
}

void MainWindow::connectUploadFile() {
  connect(uploadFile, &QPushButton::clicked, this, &MainWindow::onUploadFile);
}

void MainWindow::connectSaveImage() {
  connect(saveImage, &QPushButton::clicked, this, &MainWindow::onSaveImage);
}

void MainWindow::connectJuliaSeed() {
  connect(juliaSeed, &QPushButton::clicked, this, &MainWindow::onJuliaSeed);
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

void MainWindow::connectPower() {
  connect(powerBox,
          static_cast<void (QDoubleSpinBox::*)(double)>(
              &QDoubleSpinBox::valueChanged),
          this, &MainWindow::onPower);
}

void MainWindow::connectOctave() {
  connect(octaveBox,
          static_cast<void (QDoubleSpinBox::*)(double)>(
              &QDoubleSpinBox::valueChanged),
          this, &MainWindow::onOctave);
}

void MainWindow::connectFractal() {
  connect(fractalOption, &QComboBox::currentIndexChanged, this,
          &MainWindow::onFractal);
}

void MainWindow::connectTerrainH() {
  connect(terrainH,
          static_cast<void (QDoubleSpinBox::*)(double)>(
              &QDoubleSpinBox::valueChanged),
          this, &MainWindow::onTerrainH);
}

void MainWindow::connectTerrainS() {
  connect(terrainS,
          static_cast<void (QDoubleSpinBox::*)(double)>(
              &QDoubleSpinBox::valueChanged),
          this, &MainWindow::onTerrainS);
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

  fractalOption->setCurrentIndex(0);
  settings.twoDSpace = false;
  settings.sceneFilePath = configFilePath.toStdString();

  std::cout << "Loaded scenefile: \"" << configFilePath.toStdString() << "\"."
            << std::endl;

  realtime->sceneChanged();
}

void MainWindow::onJuliaSeed() {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> dist(-0.5, 0.5);

  double realPart = dist(gen);
  double imagPart = dist(gen);
  settings.juliaSeed = glm::vec2(realPart, imagPart);
  realtime->settingsChanged();
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

void MainWindow::onFractal(int idx) {
  std::filesystem::path curr = std::filesystem::current_path();
  settings.currentFractal = idx;
  settings.twoDSpace = false;
  switch (idx) {
  case 0:
    settings.sceneFilePath = curr.string() + "/scenefiles/simple/blank.json";
    settings.reset = true;
    break;
  case 1:
    settings.twoDSpace = true;
    settings.sceneFilePath =
        curr.string() + "/scenefiles/simple/unit_mandelbrot.json";
    break;
  case 2:
    settings.sceneFilePath =
        curr.string() + "/scenefiles/simple/unit_mandelbulb.json";
    break;
  case 3:
    settings.sceneFilePath =
        curr.string() + "/scenefiles/simple/unit_mengersponge.json";
    break;
  case 4:
    settings.sceneFilePath =
        curr.string() + "/scenefiles/simple/unit_sierpinski.json";
    break;
  }
  settings.juliaSeed = glm::vec2(0);
  realtime->sceneChanged();
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

void MainWindow::onPower(double newValue) {
  settings.power = newValue;
  realtime->settingsChanged();
}

void MainWindow::onOctave(double newValue) {
  settings.numOctaves = newValue;
  realtime->settingsChanged();
}

void MainWindow::onTerrainH(double newValue) {
  settings.terrainH = newValue;
  realtime->settingsChanged();
}

void MainWindow::onTerrainS(double newValue) {
  settings.terrainS = newValue;
  realtime->settingsChanged();
}
