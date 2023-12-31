#include "realtime.h"
#include "settings.h"
#include "utils/shaderloader.h"
#include <QCoreApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <iostream>

Realtime::Realtime(QWidget *parent) : QOpenGLWidget(parent) {
  m_prev_mouse_pos = glm::vec2(size().width() / 2, size().height() / 2);
  setMouseTracking(true);
  setFocusPolicy(Qt::StrongFocus);

  m_keyMap[Qt::Key_W] = false;
  m_keyMap[Qt::Key_A] = false;
  m_keyMap[Qt::Key_S] = false;
  m_keyMap[Qt::Key_D] = false;
  m_keyMap[Qt::Key_Control] = false;
  m_keyMap[Qt::Key_Space] = false;
}

/**
 * @brief Terminating function. Do any cleanup needed
 */
void Realtime::finish() {
  killTimer(m_timer);
  this->makeCurrent();

  // Destroy Shapes Textuers
  destroyShapesTextures();

  // Destroy Image Plane
  glDeleteVertexArrays(1, &m_imagePlaneVAO);
  glDeleteBuffers(1, &m_imagePlaneVBO);

  // Destroy Full Screen Quad
  glDeleteVertexArrays(1, &m_fullscreenVAO);
  glDeleteBuffers(1, &m_fullscreenVBO);

  // Destroy Area Light Textures
  glDeleteTextures(1, &m_mTexture);
  glDeleteTextures(1, &m_ltuTexture);

  // Destroy Defaults
  glDeleteTextures(1, &m_defaultShapeTexture);
  glDeleteTextures(1, &m_cubeMapTexture);
  glDeleteTextures(1, &m_nullCubeMapTexture);
  glDeleteTextures(1, &m_nullBloomBlurTexture);
  glDeleteTextures(1, &m_noiseTexture);
  glDeleteTextures(1, &m_blueNoiseTexture);

  // Destroy FBO
  destroyCustomFBO();

  // Destroy Shaders
  glDeleteProgram(m_rayMarchShader);
  glDeleteProgram(m_fxaaShader);
  glDeleteProgram(m_lightOptionShader);
  glDeleteProgram(m_debugShader);
  glDeleteProgram(m_blurShader);

  this->doneCurrent();
}

/**
 * @brief Initializer function
 */
void Realtime::initializeGL() {

  m_devicePixelRatio = this->devicePixelRatio();

  m_timer = startTimer(1000 / 30);
  m_elapsedTimer.start();

  glewExperimental = GL_TRUE;
  GLenum err = glewInit();
  if (err != GLEW_OK) {
    std::cerr << "Error while initializing GL: " << glewGetErrorString(err)
              << std::endl;
  }
  std::cout << "Initialized GL: Version " << glewGetString(GLEW_VERSION)
            << std::endl;

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  // Set dimensions
  scene.m_width = size().width() * m_devicePixelRatio;
  scene.m_height = size().height() * m_devicePixelRatio;
  glViewport(0, 0, scene.m_width, scene.m_height);

  // =========== SETUP =============

  // Load the shaders
  m_rayMarchShader = ShaderLoader::createShaderProgram(
      ":/resources/raymarch.vert", ":/resources/raymarch.frag");
  m_fxaaShader = ShaderLoader::createShaderProgram(
      ":/resources/fullscreen.vert", ":/resources/fxaa.frag");
  m_lightOptionShader = ShaderLoader::createShaderProgram(
      ":/resources/fullscreen.vert", ":/resources/hdr.frag");
  m_debugShader = ShaderLoader::createShaderProgram(
      ":/resources/fullscreen.vert", ":/resources/color.frag");
  m_blurShader = ShaderLoader::createShaderProgram(
      ":/resources/fullscreen.vert", ":/resources/blur.frag");

  // Initialize the image plane through which we march rays
  initImagePlane();
  // Initialize the full screen quad
  initFullScreenQuad();
  // Initialize any defaults
  initDefaults();
  // Initialize the custom FBO
  initCustomFBO();
  // Area Light Textures
  loadMTexture();
  loadLTUTexture();
  // Initialize Custom Textures
  initCustomTextures();
  // Initialize the shader
  initShader();
}

/**
 * @brief Draws the scene
 */
void Realtime::paintGL() {
  if (!scene.isInitialized()) {
    return;
  }
  // Perform Raymarch and render the scene
  rayMarch();
}

/**
 * @brief Invoked when scene is resized
 */
void Realtime::resizeGL(int w, int h) {
  glViewport(0, 0, size().width() * m_devicePixelRatio,
             size().height() * m_devicePixelRatio);
  scene.m_width = size().width() * m_devicePixelRatio;
  scene.m_height = size().height() * m_devicePixelRatio;
  if (!scene.isInitialized()) {
    return;
  }
  // Resize the scene and update the camera
  scene.resizeScene(scene.m_width, scene.m_height);
  // Destroy and re-Init FBO
  destroyCustomFBO();
  initCustomFBO();
}

/**
 * @brief Invoked when a new scene file is uploaded
 */
void Realtime::sceneChanged() {
  if (scene.isInitialized()) {
    // Destroy previous shapes textures
    destroyShapesTextures();
    m_isAreaLightUsed = false;
  }
  if (settings.reset) {
    scene.resetScene();
    settings.reset = false;
    return;
  }
  // Initialize the Raymarch scene
  scene.initScene(settings, m_isAreaLightUsed);
  // Initialize the textures
  initShapesTextures();
  // Clear the seed
  m_juliaSeed = glm::vec2(0.f);
  // Update the dim
  m_twoDSpace = settings.twoDSpace;
  update();
}

/**
 * @brief Invoked whenever any of the ui settings have been modified
 */
void Realtime::settingsChanged() {
  if (!scene.isInitialized()) {
    return;
  }
  // Update the camera
  scene.updateScene(settings);
  // Update the options
  updateUISettings();
  update();
}

void Realtime::keyPressEvent(QKeyEvent *event) {
  m_keyMap[Qt::Key(event->key())] = true;
}

void Realtime::keyReleaseEvent(QKeyEvent *event) {
  m_keyMap[Qt::Key(event->key())] = false;
}

void Realtime::mousePressEvent(QMouseEvent *event) {
  if (event->buttons().testFlag(Qt::LeftButton)) {
    m_mouseDown = true;
    m_prev_mouse_pos = glm::vec2(event->position().x(), event->position().y());
  }
}

void Realtime::mouseReleaseEvent(QMouseEvent *event) {
  if (!event->buttons().testFlag(Qt::LeftButton)) {
    m_mouseDown = false;
  }
}

void Realtime::mouseMoveEvent(QMouseEvent *event) {
  if (m_mouseDown) {
    int posX = event->position().x();
    int posY = event->position().y();
    int deltaX = posX - m_prev_mouse_pos.x;
    int deltaY = posY - m_prev_mouse_pos.y;
    m_prev_mouse_pos = glm::vec2(posX, posY);

    if (!scene.isInitialized()) {
      return;
    }

    if (deltaX == 0 && deltaY == 0) {
      return;
    }

    Camera &cam = scene.getCamera();
    cam.rotateX(deltaX);
    cam.rotateY(deltaY);

    update();
  }
}

void Realtime::timerEvent(QTimerEvent *event) {
  int elapsedms = m_elapsedTimer.elapsed();
  float deltaTime = elapsedms * 0.001f;
  m_delta += deltaTime;
  m_elapsedTimer.restart();

  if (!scene.isInitialized()) {
    return;
  }

  float s = deltaTime * 5.f;
  glm::vec3 disp = glm::vec3(0.f);
  Camera &cam = scene.getCamera();

  // W
  if (m_keyMap[Qt::Key_W]) {
    disp += cam.onWPressed();
  }
  // A
  if (m_keyMap[Qt::Key_A]) {
    disp += cam.onAPressed();
  }
  // D
  if (m_keyMap[Qt::Key_D]) {
    disp += cam.onDPressed();
  }
  // S
  if (m_keyMap[Qt::Key_S]) {
    disp += cam.onSPressed();
  }
  // Space
  if (m_keyMap[Qt::Key_Space]) {
    disp += cam.onSpacePressed();
  }
  // Ctrl
  if (m_keyMap[Qt::Key_Control]) {
    disp += cam.onControlPressed();
  }

  // Check if moved
  if (glm::length(disp) != 0.f) {
    disp *= s;
    cam.applyTranslation(disp);
  }

  update();
}

// DO NOT EDIT
void Realtime::saveViewportImage(std::string filePath) {
  // Make sure we have the right context and everything has been drawn
  makeCurrent();

  int fixedWidth = 1024;
  int fixedHeight = 768;

  // Create Frame Buffer
  GLuint fbo;
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);

  // Create a color attachment texture
  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fixedWidth, fixedHeight, 0, GL_RGB,
               GL_UNSIGNED_BYTE, NULL);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         texture, 0);

  // Optional: Create a depth buffer if your rendering uses depth testing
  GLuint rbo;
  glGenRenderbuffers(1, &rbo);
  glBindRenderbuffer(GL_RENDERBUFFER, rbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, fixedWidth,
                        fixedHeight);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, rbo);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    std::cerr << "Error: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return;
  }

  // Render to the FBO
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glViewport(0, 0, fixedWidth, fixedHeight);

  // Clear and render your scene here
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  paintGL();

  // Read pixels from framebuffer
  std::vector<unsigned char> pixels(fixedWidth * fixedHeight * 3);
  glReadPixels(0, 0, fixedWidth, fixedHeight, GL_RGB, GL_UNSIGNED_BYTE,
               pixels.data());

  // Unbind the framebuffer to return to default rendering to the screen
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // Convert to QImage
  QImage image(pixels.data(), fixedWidth, fixedHeight, QImage::Format_RGB888);
  QImage flippedImage = image.mirrored(); // Flip the image vertically

  // Save to file using Qt
  QString qFilePath = QString::fromStdString(filePath);
  if (!flippedImage.save(qFilePath)) {
    std::cerr << "Failed to save image to " << filePath << std::endl;
  }

  // Clean up
  glDeleteTextures(1, &texture);
  glDeleteRenderbuffers(1, &rbo);
  glDeleteFramebuffers(1, &fbo);
}
