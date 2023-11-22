#pragma once

#include "scenedata.h"
#include <string>
#include <vector>

// Struct which contains data for a single primitive, to be used for rendering
struct RenderShapeData {
  ScenePrimitive primitive;
  glm::mat4 ctm; // the cumulative transformation matrix
};

// Struct which contains all the data needed to render a scene
struct RenderData {
  SceneGlobalData globalData;
  SceneCameraData cameraData;

  std::vector<SceneLightData> lights;
  std::vector<RenderShapeData> shapes;
};

class SceneParser {
private:
  // Given a lignt of type SceneLight, return SceneLightData with ctm applied
  static SceneLightData getSceneLightDataFromSceneLight(const SceneLight &light,
                                                        glm::mat4 ctm);

  // Given a list of transformations, apply all of them in order and return the
  // total transformation
  static glm::mat4
  getLocTransMat(const std::vector<SceneTransformation *> trans,
                 glm::mat4 parent);

  // Recursive helper function for parsing the scene graph
  static void parseHelper(RenderData &renderData, SceneNode *currScene,
                          glm::mat4 parent);

public:
  // Parse the scene and store the results in renderData.
  static bool parse(std::string filepath, RenderData &renderData);
};
