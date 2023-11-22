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

/**
 * @struct Settings
 * Stores application settings for GUI.
 */
struct Settings {
  std::string sceneFilePath;
  float nearPlane;
  float farPlane;
  int screenWidth = 1024;
  int screenHeight = 768;
};

// The global Settings object, will be initialized by MainWindow
extern Settings settings;

#endif // SETTINGS_H
