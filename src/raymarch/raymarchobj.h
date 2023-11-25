#ifndef RAYMARCHOBJ_H
#define RAYMARCHOBJ_H

#include "utils/scenedata.h"

struct RayMarchObj {
  // Struct that represents a single object

  // Cstr
  RayMarchObj(int id, PrimitiveType t, const glm::mat4 &ctm,
              const glm::mat4 &scale, const SceneMaterial &mat)
      : m_id(id), m_type(t), m_ctm(ctm), m_scale(scale),
        m_ctmInv(glm::inverse(ctm)), m_material(mat) {}

  // Unique ID for this object
  int m_id;
  // Type
  PrimitiveType m_type;
  // CTM (obj -> world)
  glm::mat4 m_ctm;
  // Accumulated scale
  // - need this to offset the non-rigid transform side-effects
  glm::mat4 m_scale;
  // Inv CTM (world -> obj)
  glm::mat4 m_ctmInv;
  // Material
  SceneMaterial m_material;
  // Texture
  uint m_texture;
};

#endif // RAYMARCHOBJ_H
