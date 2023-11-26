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
  // Struct that contains everything in the scene

public:
  // PUBLIC METHODS

  // Initializes the Scene given a scene json file
  void initScene(Settings &from);

  // Updates the scene with new camera paramters
  void updateScene(Settings &from);

  // Resizes the scene with new dimensions
  void resizeScene(int newWidth, int newHeight);

  // Returns True if scene is initialized
  bool isInitialized() const;

  // Resets Camera for scene change
  void resetCamera();

  // Gets Shapes
  std::vector<RayMarchObj> &getShapes();

  // Gets Shapes Textures
  std::map<std::string, TextureInfo> &getShapesTextures();

  // Gets Lights
  std::vector<SceneLightData> &getLights();

  // Gets Camera
  Camera &getCamera();

  // Gets Global Data
  SceneGlobalData &getGlobalData();

  // Gets the file paths for cube map based on the type
  std::vector<std::string> getCubeMapWithType(CUBEMAP type);

private:
  // PRIVATE METHODS

  // Initializes objects read from json
  void initRayMarchObjs(std::map<std::string, TextureInfo> &textureMap,
                        std::vector<RenderShapeData> &rd);

  // Loads texture if used
  void loadTextureFromPrim(std::map<std::string, TextureInfo> &out,
                           const std::string &file);

private:
  // PRIVATE MEMBERS

  // Status of the scene. True if initialized
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
