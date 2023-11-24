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

/**
 * @brief Function that gets the shapes texture data of the scene
 * @returns map from texture file name to data
 */
std::map<std::string, TextureInfo> &RayMarchScene::getShapesTextures() {
  return m_textures;
}

/**
 * @brief Function that gets the lights in the scene
 * @returns vector containing SceneLightData
 */
std::vector<SceneLightData> &RayMarchScene::getLights() { return m_lights; }

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
  initRayMarchObjs(m_textures, rd.shapes);
  // ---- LIghts ----
  m_lights = rd.lights;
}

/**
 * @brief Function that resizes the scene
 * This is called in resizeGL() function
 * @param w New width
 * @param h New height
 */
void RayMarchScene::resizeScene(int w, int h) {
  m_camera.updateCameraDimensions(w, h);
}

/**
 * @brief Function that updates the scene based on the settings
 * This is called in settingsChanged() function with the updated settings passed
 * in
 * @param s Settings that is new
 */
void RayMarchScene::updateScene(Settings &s) {
  // Update the camera, if necessary
  m_camera.updateCamera(s);
}

/**
 * @brief Function that resets the camera
 * Called in
 *   - RealTime::sceneChanged()
 */
void RayMarchScene::resetCamera() { m_camera = Camera{}; };

/**
 * @brief Function that initializes our RayMarch objs given a vec ofk
 * RenderShapeData
 */
void RayMarchScene::initRayMarchObjs(
    std::map<std::string, TextureInfo> &textureMap,
    std::vector<RenderShapeData> &rd) {
  textureMap.clear();
  m_shapes.clear();
  m_shapes.reserve(rd.size());
  int id = 0;
  for (const RenderShapeData &shapeData : rd) {
    // If texture is used, load up here
    if (shapeData.primitive.material.textureMap.isUsed) {
      loadTextureFromPrim(textureMap,
                          shapeData.primitive.material.textureMap.filename);
    }
    m_shapes.emplace_back(id, shapeData.primitive.type, shapeData.ctm,
                          shapeData.scale, shapeData.primitive.material);
    id++;
  }
}

/**
 * @brief Load the texture given by "file" to our map
 * @param file Filepath we are loading from
 */
void RayMarchScene::loadTextureFromPrim(std::map<std::string, TextureInfo> &out,
                                        const std::string &file) {
  // Check if we already have this file loaded
  if (auto search = out.find(file); search != out.end()) {
    // Exists
    return;
  }
  // Load up
  QImage myImage;
  QString str(file.data());
  if (!myImage.load(str)) {
    std::cout << "Failed to load in image" << std::endl;
    return;
  }
  myImage = myImage.convertToFormat(QImage::Format_RGBA8888).mirrored();
  auto width = myImage.width();
  auto height = myImage.height();
  QByteArray arr = QByteArray::fromRawData((const char *)myImage.bits(),
                                           myImage.sizeInBytes());
  std::vector<RGBA> output;
  output.clear();
  output.reserve(width * height);
  for (int i = 0; i < arr.size() / 4.f; i++) {
    output.push_back(
        RGBA{(std::uint8_t)arr[4 * i], (std::uint8_t)arr[4 * i + 1],
             (std::uint8_t)arr[4 * i + 2], (std::uint8_t)arr[4 * i + 3]});
  }
  // Add to our map
  out[file] = TextureInfo{
      myImage,
      output,
      width,
      height,
  };
}
