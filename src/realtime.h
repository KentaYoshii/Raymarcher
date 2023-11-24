#pragma once

// Defined before including GLEW to suppress deprecation messages on macOS
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
#include <GL/glew.h>
#include <glm/glm.hpp>

#include "raymarch/raymarchscene.h"
#include <QElapsedTimer>
#include <QOpenGLWidget>
#include <QTime>
#include <QTimer>
#include <unordered_map>

class Realtime : public QOpenGLWidget {
public:
  Realtime(QWidget *parent = nullptr);
  void finish(); // Called on program exit
  void sceneChanged();
  void settingsChanged();
  void saveViewportImage(std::string filePath);

public slots:
  void tick(QTimerEvent *event); // Called once per tick of m_timer

protected:
  void initializeGL() override; // Called once at the start of the program
  void paintGL() override; // Called whenever the OpenGL context changes or by
                           // an update() request
  void resizeGL(int width,
                int height) override; // Called when window size changes

private:
  void keyPressEvent(QKeyEvent *event) override;
  void keyReleaseEvent(QKeyEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void timerEvent(QTimerEvent *event) override;

  // Tick Related Variables
  int m_timer; // Stores timer which attempts to run ~60 times per second
  QElapsedTimer m_elapsedTimer; // Stores timer which keeps track of actual time
                                // between frames

  // Input Related Variables
  bool m_mouseDown = false;   // Stores state of left mouse button
  glm::vec2 m_prev_mouse_pos; // Stores mouse position
  std::unordered_map<Qt::Key, bool>
      m_keyMap; // Stores whether keys are pressed or not

  // Device Correction Variables
  int m_devicePixelRatio;

  // ============ RAY MARCHER ==============

  // PRIVATE DATA

  // Our RayMarch scene
  RayMarchScene scene;

  // Shader
  GLuint m_rayMarchShader;

  // Textures
  GLuint m_defaultShapeTexture;

  // FBO
  GLuint m_defaultFBO = 1;

  // Image Plane
  GLuint m_imagePlaneVAO;
  GLuint m_imagePlaneVBO;

  // Toggelable Options
  bool m_enableGammaCorrection;
  bool m_enableSoftShadow;

  // PRIVATE METHODS
  void rayMarch();

  void initShader();
  void initDefaults();
  void initImagePlane();
  void initShapesTextures();

  void setFBO(GLuint fbo);

  void draw(GLuint shader);

  void configureScreenUniforms(GLuint shader);
  void configureCameraUniforms(GLuint shader);
  void configureShapesUniforms(GLuint shader);
  void configureLightsUniforms(GLuint shader);
  void configureSettingsUniforms(GLuint shader);
};
