#include "raymarchscene.h"
#include "utils/sceneparser.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <random>

/**
 * @brief Function that gets the shapes in the scene
 * @returns vector containing RealTimeShapes
 */
std::vector<RayMarchObj> &RayMarchScene::getShapes() { return m_shapes; }

///**
// * @brief Function that gets the lights in the scene
// * @returns vector containing SceneLightData
// */
// std::vector<SceneLightData> &RealTimeScene::getLights() { return m_lights; }

/**
 * @brief Gets the status of the scene
 * @returns True if the scene is initialized
 */
bool RayMarchScene::isInitialized() const { return m_init; }

/**
 * @brief Function that gets the Camera in the scene
 * @returns Camera
 */
Camera &RayMarchScene::getCamera() { return m_camera; }

/**
 * @brief Function that gets the global data of the scene
 * @returns SceneGlobalData
 */
SceneGlobalData &RayMarchScene::getGlobalData() { return m_globalData; }

/**
 * @brief Function that initializes the scene for RealTime
 * This is called in sceneChanged() function with the new json file.
 * 1. Parse the scene json file
 * 2. Set up the scene
 *    - Global Data
 *    - Camera
 *    - Lights
 *    - Shapes
 * @param from Latest Settings at the time of reading the scene json file
 */
void RayMarchScene::initScene(Settings &from) {

  m_init = true;

  // First, parse the scene json in to RenderData
  RenderData rd;
  SceneParser::parse(from.sceneFilePath, rd);

  // Then we construct our scene one by one

  // ---- Global Data ----
  m_globalData = rd.globalData;

  // ---- Camera ----
  m_camera.initializeCamera(rd.cameraData, from);

  // ---- Shapes ----
  initRayMarchObjs(rd.shapes);
}

/**
 * @brief Function that resets the camera
 * Called in
 *   - RealTime::sceneChanged()
 */
void RayMarchScene::resetCamera() { m_camera = Camera{}; };

/**
 * @brief Function that initializes our RayMarch objs given a vec of
 * RenderShapeData
 */
void RayMarchScene::initRayMarchObjs(std::vector<RenderShapeData> &rd) {
  m_shapes.clear();
  m_shapes.reserve(rd.size());
  int id = 0;
  for (const RenderShapeData &shapeData : rd) {
    RayMarchObj obj{};
    obj.m_id = id;
    obj.m_type = shapeData.primitive.type;
    obj.m_ctm = shapeData.ctm;
    obj.m_ctmInv = glm::inverse(shapeData.ctm);
    obj.m_material = shapeData.primitive.material;
    m_shapes.push_back(obj);
    id++;
  }
}
