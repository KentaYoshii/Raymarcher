#include "realtime.h"

void Realtime::initImagePlane() {
  // Four corners of Image Plane
  std::vector<GLfloat> verts = {
      -1.f, 1.f,  // TL
      -1.f, -1.f, // BL
      1.f,  -1.f, // BR
      1.f,  1.f,  // TR
      -1.f, 1.f,  // TL
      1.f,  -1.f, // BR
  };

  // VBO
  glGenBuffers(1, &m_imagePlaneVBO);
  glBindBuffer(GL_ARRAY_BUFFER, m_imagePlaneVBO);
  glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(GLfloat), verts.data(),
               GL_STATIC_DRAW);

  // VAO
  glGenVertexArrays(1, &m_imagePlaneVAO);
  glBindVertexArray(m_imagePlaneVAO);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}
