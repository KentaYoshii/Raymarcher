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

  // Update the scene with new camera paramters
  void updateScene(Settings &from);

  // Resize the scene with new dimensions
  void resizeScene(int newWidth, int newHeight);

  // True if scene is initialized
  bool isInitialized() const;

  // Reset Camera for scene change
  void resetCamera();

  // Get Shapes
  std::vector<RayMarchObj> &getShapes();

  // Get Shapes Textures
  std::map<std::string, TextureInfo> &getShapesTextures();

  // Get Lights
  std::vector<SceneLightData> &getLights();

  // Get Camera
  Camera &getCamera();

  // Get Global Data
  SceneGlobalData &getGlobalData();

private:
  // PRIVATE METHODS

  // Initialize objects read from json
  void initRayMarchObjs(std::map<std::string, TextureInfo> &textureMap,
                        std::vector<RenderShapeData> &rd);

  // Load texture if used
  void loadTextureFromPrim(std::map<std::string, TextureInfo> &out,
                           const std::string &file);

private:
  // PRIVATE MEMBERS

  bool m_init = false;

  // Global Data
  SceneGlobalData m_globalData;

  // Camera
  Camera m_camera;

  // Shapes
  std::vector<RayMarchObj> m_shapes;

  // Shapes Textures
  std::map<std::string, TextureInfo> m_textures;

  // Lights
  std::vector<SceneLightData> m_lights;

public:
  // Screen Dimension
  int m_width;
  int m_height;
};

#endif // RAYMARCHSCENE_H
