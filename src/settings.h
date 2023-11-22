/**
 * @file    Settings.h
 *
 * This file contains various settings and enumerations that you will need to
 * use in the various assignments. The settings are bound to the GUI via static
 * data bindings.
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include "utils/rgba.h"
#include <QObject>

// Enumeration values for the Brush types from which the user can choose in the
// GUI.
enum BrushType {
  BRUSH_CONSTANT,
  BRUSH_LINEAR,
  BRUSH_QUADRATIC,
  BRUSH_SMUDGE,
  BRUSH_SPRAY,
  BRUSH_SPEED,
  BRUSH_FILL,
  BRUSH_CUSTOM,
  NUM_BRUSH_TYPES
};

// Enumeration values for the Filters that the user can select in the GUI.
enum FilterType {
  FILTER_EDGE_DETECT,
  FILTER_BLUR,
  FILTER_SCALE,
  FILTER_MEDIAN,
  FILTER_CHROMATIC,
  FILTER_MAPPING,
  FILTER_ROTATION,
  FILTER_BILATERAL,
  NUM_FILTER_TYPES
};

/**
 * @struct Settings
 *
 * Stores application settings for the CS123 GUI.
 *
 * You can access all app settings through the "settings" global variable.
 * The settings will be automatically updated when things are changed in the
 * GUI (the reverse is not true however: changing the value of a setting does
 * not update the GUI).
 */
struct Settings {
  // Brush
  int brushType;   // The user's selected brush @see BrushType
  int brushRadius; // The brush radius
  RGBA brushColor;
  int brushDensity;      // This is for spray brush (extra credit)
  bool fixAlphaBlending; // Fix alpha blending (extra credit)

  // Filter
  int filterType;              // The selected filter @see FilterType
  float edgeDetectSensitivity; // Edge detection sensitivity, from 0 to 1.
  int blurRadius;              // Selected blur radius
  float scaleX;                // Horizontal scale factor
  float scaleY;                // Vertical scale factor
  int medianRadius;            // Median radius (extra credit)
  float rotationAngle;         // Rotation angle (extra credit)
  int bilateralRadius;         // Bilateral radius (extra credit)
  int rShift;        // Chromatic aberration red channel shift (extra credit)
  int gShift;        // Chromatic aberration green channel shift (extra credit)
  int bShift;        // Chromatic aberration blue channel shift (extra credit)
  bool nonLinearMap; // Use non-linear mapping function for tone mapping (extra
                     // credit)
  float gamma;       // Gamma for tone mapping (extra credit)

  QString imagePath;
};

// The global Settings object, will be initialized by MainWindow
extern Settings settings;

#endif // SETTINGS_H
