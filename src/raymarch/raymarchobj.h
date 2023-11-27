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

  // Area light cstr
  RayMarchObj(int id, PrimitiveType t, const glm::mat4 &ctm, const glm::vec4 &c)
      : m_id(id), m_type(t), m_ctm(ctm), m_scale(glm::mat4(0.1)),
        m_ctmInv(glm::inverse(ctm)), m_isEmissive(true), m_color(c),
        m_material(SceneMaterial{glm::vec4(0), glm::vec4(0), glm::vec4(0), 0.0,
                                 glm::vec4(0), glm::vec4(0), 0, SceneFileMap{},
                                 0, glm::vec4(0), SceneFileMap{}}) {}

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

  // Area Light
  bool m_isEmissive = false;
  glm::vec4 m_color = glm::vec4(0.f);
};

#endif // RAYMARCHOBJ_H
