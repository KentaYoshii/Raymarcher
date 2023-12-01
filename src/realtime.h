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

#define MAX_NUM_LIGHTS 10
#define MAX_NUM_TEXTURES 10
#define MAX_NUM_SHAPES 30
#define SKYBOX_TEX_UNIT_OFF 10
#define LTC1_TEX_UNIT_OFF 11
#define LTC2_TEX_UNIT_OFF 12
#define BLOOM_BLUR_COUNT 10

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
  float m_delta;

  // Input Related Variables
  bool m_mouseDown = false;   // Stores state of left mouse button
  glm::vec2 m_prev_mouse_pos; // Stores mouse position
  std::unordered_map<Qt::Key, bool>
      m_keyMap; // Stores whether keys are pressed or not

  // Device Correction Variables
  int m_devicePixelRatio;

  // ============ RAY MARCHER ==============

  // PRIVATE DATA

  // RayMarch scene
  RayMarchScene scene;

  // Shader
  // - raymarch shader
  GLuint m_rayMarchShader;
  // - fxaa shader
  GLuint m_fxaaShader;
  // - hdr shader
  GLuint m_lightOptionShader;
  // - debug shader
  GLuint m_debugShader;
  // - Bloom (blur shader)
  GLuint m_blurShader;

  // Textures
  // - default material texture
  GLuint m_defaultShapeTexture;
  // - map from file name to texture object
  std::unordered_map<std::string, GLuint> m_TextureMap;
  // - hdr texture
  GLuint m_hdrTexture;
  // - Bloom
  GLuint m_bloomBrightnessTexture;
  GLuint m_nullBloomBlurTexture;
  // - cube map texture
  GLuint m_cubeMapTexture;
  // - null cube map texture
  GLuint m_nullCubeMapTexture;

  // FBO
  // - application window FBO
  GLuint m_defaultFBO = 4;
  // - custom FBO
  GLuint m_customFBO;
  GLuint m_customFBOColorTexture;
  GLuint m_customFBORenderBuffer;
  // - Bloom
  GLuint m_pingpongFBO[2];
  GLuint m_pingpongBuffer[2];

  // Image Plane through which we march rays
  GLuint m_imagePlaneVAO;
  GLuint m_imagePlaneVBO;

  // Full Screen Quad for post processing
  GLuint m_fullscreenVAO;
  GLuint m_fullscreenVBO;

  // Area Light
  // source: https://learnopengl.com/Guest-Articles/2022/Area-Lights
  bool m_isAreaLightUsed = false;
  GLuint m_mTexture;
  GLuint m_ltuTexture;
  void loadMTexture();
  void loadLTUTexture();
  const std::vector<glm::vec3> corners = {
      glm::vec3(-0.5f, 0.5f, 0.f),  // tl
      glm::vec3(0.5f, 0.5f, 0.f),   // tr
      glm::vec3(0.5f, -0.5f, 0.f),  // br
      glm::vec3(-0.5f, -0.5f, 0.f), // bl
  };

  // Toggelable Options

  // Others
  bool m_twoDSpace = false;

  // Render Effects
  // - soft shadow
  bool m_enableSoftShadow;
  // - reflection
  bool m_enableReflection;
  // - refraction
  bool m_enableRefraction;
  // - ambient occulusion
  bool m_enableAmbientOcclusion;
  // - sky box
  int m_idxSkyBox;
  // Post Processing Effects
  // - FXAA
  bool m_enableFXAA;

  // Lighting Effects
  // - exposure
  float m_exposure;
  // - HDR
  bool m_enableHDR;
  // - Bloom
  bool m_enableBloom;
  // - gamma correction
  bool m_enableGammaCorrection;

  // Fractals
  // - power of fractals
  float m_power = 8.f;
  // - julia seed (real and imaginary components)
  glm::vec2 m_juliaSeed = glm::vec2(0.f);

  // PRIVATE METHODS

  // Performs raymarching using our raymarch shader
  void rayMarch();
  // Applies FXAA post processing
  void applyFXAA();
  // Applies HDR post processing
  void applyLightEffects();
  // Applies Bloom Post processing
  bool applyBloom();
  // Draws to the fullsreen quad with given tex
  void drawToQuadWithTex(GLuint tex);

  // Initializes the shaders with constant uniforms
  void initShader();
  // Initializes all the default variables used in shader
  void initDefaults();
  // Initializes [-1,1] blank canvas to be used for raymarching
  void initImagePlane();
  // Initializes full screen quad
  void initFullScreenQuad();
  // Initializes each and every material texture used in the scene
  void initShapesTextures();
  // Initializes our custom FBO for offline rendering
  void initCustomFBO();
  // Initializes our cube map
  void initCubeMap(CUBEMAP type);

  // Sets the output FBO
  void setFBO(GLuint fbo);

  // Update settings
  void updateUISettings();

  // Sets the uniforms for our screen-related stuff
  void configureScreenUniforms(GLuint shader);
  // Sets the uniforms for camera-related stuff
  void configureCameraUniforms(GLuint shader);
  // Sets the uniforms for each shape in the scene
  void configureShapesUniforms(GLuint shader);
  // Sets the uniforms for each light in the scene
  void configureLightsUniforms(GLuint shader);
  // Sets the uniforms for all the rendering options
  void configureSettingsUniforms(GLuint shader);
  // Sets the uniforms for FXAA
  void configureFXAAUniforms(GLuint shader);
  // Sets the uniforms for applying light efects
  void configureLightEffectsUniforms(GLuint shader, bool side);

  // Destroies shapes textures
  void destroyShapesTextures();
  // Destroy custom FBO
  void destroyCustomFBO();

  // Utility
  void setIntUniform(GLuint shader, const char *, int val);
  void setFloatUniform(GLuint shader, const char *, float val);
  void setMat4Uniform(GLuint shader, const char *, const glm::mat4 &);
  void setVec2Uniform(GLuint shader, const char *, const glm::vec2 &);
  void setVec3Uniform(GLuint shader, const char *, const glm::vec3 &);
  void setVec4Uniform(GLuint shader, const char *, const glm::vec4 &);
};
