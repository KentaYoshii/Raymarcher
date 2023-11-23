#ifndef RAYMARCHOBJ_H
#define RAYMARCHOBJ_H

#include "utils/scenedata.h"

struct RayMarchObj {
  // Struct that represents a single object

  // Unique ID for this object
  int m_id;
  // Type
  PrimitiveType m_type;
  // CTM (obj -> world)
  glm::mat4 m_ctm;
  // TOTAL SCALE
  glm::mat4 m_scale;
  // Inv CTM (world -> obj)
  glm::mat4 m_ctmInv;
  // Material
  SceneMaterial m_material;
};

#endif // RAYMARCHOBJ_H
