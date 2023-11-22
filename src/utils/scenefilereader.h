#pragma once

#include "scenedata.h"

#include <map>
#include <vector>

#include <QJsonDocument>
#include <QJsonObject>

// This class parses the scene graph specified by the CS123 Xml file format.
class ScenefileReader {
public:
  // Create a ScenefileReader, passing it the scene file.
  ScenefileReader(const std::string &filename);

  // Clean up all data for the scene
  ~ScenefileReader();

  // Parse the XML scene file. Returns false if scene is invalid.
  bool readJSON();

  SceneGlobalData getGlobalData() const;

  SceneCameraData getCameraData() const;

  SceneNode *getRootNode() const;

private:
  // The filename should be contained within this parser implementation.
  // If you want to parse a new file, instantiate a different parser.
  bool parseGlobalData(const QJsonObject &globaldata);
  bool parseCameraData(const QJsonObject &cameradata);
  bool parseTemplateGroups(const QJsonValue &templateGroups);
  bool parseTemplateGroupData(const QJsonObject &templateGroup);
  bool parseGroups(const QJsonValue &groups, SceneNode *parent);
  bool parseGroupData(const QJsonObject &object, SceneNode *node);
  bool parsePrimitive(const QJsonObject &prim, SceneNode *node);
  bool parseLightData(const QJsonObject &lightData, SceneNode *node);

  std::string file_name;

  mutable std::map<std::string, SceneNode *> m_templates;

  SceneGlobalData m_globalData;
  SceneCameraData m_cameraData;

  SceneNode *m_root;
  std::vector<SceneNode *> m_nodes;
};
