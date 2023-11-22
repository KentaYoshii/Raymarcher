#include "sceneparser.h"
#include "scenefilereader.h"
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>

#include <chrono>

/**
 * @brief Given a "light", return corresponding SceneLightData after "ctm" is
 * applied
 * @param light: light we wish to transform
 * @param ctm: cumulative transformatin matrix to be applied
 * @return SceneLightData which is "light" after applying "ctm"
 */
SceneLightData
SceneParser::getSceneLightDataFromSceneLight(const SceneLight &light,
                                             glm::mat4 ctm) {
  return SceneLightData{
      light.id,
      light.type,
      light.color,
      light.function,
      ctm * glm::vec4(0.f, 0.f, 0.f, 1.f),
      ctm * light.dir,
      light.penumbra,
      light.angle,
      light.width,
      light.height,
  };
}

/**
 * @brief Given a vector of "trans", return the total transformation matrix
 * @param trans: vector of SceneTransformation
 * @return glm::mat4 which is the total transformation
 */
glm::mat4
SceneParser::getLocTransMat(const std::vector<SceneTransformation *> trans,
                            glm::mat4 parent) {
  glm::mat4 T = glm::mat4(1.f);
  glm::mat4 R = glm::mat4(1.f);
  glm::mat4 S = glm::mat4(1.f);
  glm::mat4 totalCTM = glm::mat4(1.f);
  for (int i = trans.size() - 1; i >= 0; i--) {
    glm::mat4 currTrans;
    switch (trans[i]->type) {
    case TransformationType::TRANSFORMATION_ROTATE: {
      if (trans[i]->rotate == glm::vec3(0, 0, 0)) {
        continue;
      }
      R = R * glm::rotate(trans[i]->angle, trans[i]->rotate);
      break;
    }
    case TransformationType::TRANSFORMATION_SCALE: {
      S = glm::scale(S, trans[i]->scale);
      break;
    }
    case TransformationType::TRANSFORMATION_TRANSLATE: {
      T = glm::translate(T, trans[i]->translate);
      break;
    }
    case TransformationType::TRANSFORMATION_MATRIX: {
      totalCTM = trans[i]->matrix;
      break;
    }
    }
  }
  return parent * totalCTM * T * R * S;
}

/**
 * @brief Given a SceneNode "currScene" and ctm "parent", apply ctm to each
 * object in our primitive list. Also apply ctm to lights. Store all of them in
 * "renderData".
 * @param renderData: RenderData we are storing our outputs to
 * @param currScene: currScene we are working with
 * @param parent: parent node's ctm
 */
void SceneParser::parseHelper(RenderData &renderData, SceneNode *currScene,
                              glm::mat4 parent) {
  // First we find the local transformation matrix
  glm::mat4 ctm = getLocTransMat(currScene->transformations, parent);
  // Then compute the CTM of this node
  // For each primitive
  for (int i = 0; i < currScene->primitives.size(); i++) {
    renderData.shapes.push_back(RenderShapeData{
        *currScene->primitives[i],
        ctm,
    });
  }
  // For each light, apply ctm
  for (int i = 0; i < currScene->lights.size(); i++) {
    auto sceneLightData =
        getSceneLightDataFromSceneLight(*currScene->lights[i], ctm);
    renderData.lights.push_back(sceneLightData);
  }
  // For each child scene, recursively call this function
  for (int i = 0; i < currScene->children.size(); i++) {
    parseHelper(renderData, currScene->children[i], ctm);
  }
}

/** Parse the scene and store the results in renderData.
 * @param filepath    The path of the scene file to load.
 * @param renderData  On return, this will contain the metadata of the loaded
 * scene.
 * @return            A boolean value indicating whether the parse was
 * successful
 */
bool SceneParser::parse(std::string filepath, RenderData &renderData) {
  ScenefileReader fileReader = ScenefileReader(filepath);
  bool success = fileReader.readJSON();
  if (!success) {
    return false;
  }
  renderData.globalData = fileReader.getGlobalData();
  renderData.cameraData = fileReader.getCameraData();
  auto rt = fileReader.getRootNode();
  // clean slate
  renderData.shapes.clear();
  renderData.lights.clear();
  // start the parsign from the root
  parseHelper(renderData, rt, glm::mat4(1.0f));

  return true;
}
