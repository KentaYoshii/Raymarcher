#ifndef RAYMARCHSCENE_H
#define RAYMARCHSCENE_H

#include "camera/camera.h"
#include "raymarch/raymarchobj.h"
#include "settings.h"
#include "utils/sceneparser.h"
#include <map>
#include <string>
#include <tuple>

struct RayMarchScene {
  // Struct that is a wrapper for everything in the scene

public:
  // PUBLIC METHODS

  // Initialize the Scene given a scene json file
  // - called in realtime::sceneChanged()
  void initScene(Settings &from);

  // True if scene is initialized
  bool isInitialized() const;

  // Reset Camera for scene change
  void resetCamera();

  // Get Shapes
  std::vector<RayMarchObj> &getShapes();

  // Get Camera
  Camera &getCamera();

  // Get Global Data
  SceneGlobalData &getGlobalData();

private:
  // PRIVATE METHODS

  // Initialize objects read from json
  void initRayMarchObjs(std::vector<RenderShapeData> &rd);

private:
  // PRIVATE MEMBERS

  bool m_init = false;

  // Global Data
  SceneGlobalData m_globalData;

  // Camera
  Camera m_camera;

  // Shapes
  std::vector<RayMarchObj> m_shapes;

  // Lights
  // std::vector<SceneLightData> m_lights;

public:
  // Screen Dimension
  int m_width;
  int m_height;
};

#endif // RAYMARCHSCENE_H
