/**
 * @file    Settings.h
 *
 * This file contains various settings and enumerations that you will need to
 * use in the various assignments. The settings are bound to the GUI via static
 * data bindings.
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <glm/glm.hpp>

/**
 * @struct Settings
 * Stores application settings for GUI.
 */
struct Settings {
  std::string sceneFilePath;
  int screenWidth = 1024;
  int screenHeight = 768;

  bool reset;
  bool twoDSpace;

  // Camera Options
  float nearPlane;
  float farPlane;

  // Render Options
  bool enableSoftShadow;
  bool enableReflection;
  bool enableRefraction;
  bool enableAmbientOcculusion;
  // Post Processing Options
  bool enableFXAA;
  bool enableGammaCorrection;
  bool enableHDR;
  bool enableBloom;
  double exposure;
  // Sky Box
  int idxSkyBox;
  // Fractals
  int currentFractal;
  float power = 8.f;
  glm::vec2 juliaSeed = glm::vec2(0.f);
  // Procedural
  int numOctaves = 5;
  float terrainH = 5.;
  float terrainS = 9.5;
};

// The global Settings object, will be initialized by MainWindow
extern Settings settings;

#endif // SETTINGS_H
