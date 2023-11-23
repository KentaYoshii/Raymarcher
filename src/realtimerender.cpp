#include "realtime.h"

void Realtime::rayMarch() {
  // Set ray march shader
  glUseProgram(m_rayMarchShader);
  // Set FBO
  // setFBO(m_defaultFBO);
  // Set Uniforms
  configureScreenUniforms(m_rayMarchShader);
  configureCameraUniforms(m_rayMarchShader);
  // Draw
  glBindVertexArray(m_imagePlaneVAO);
  draw(m_rayMarchShader);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  // Un-set
  glBindVertexArray(0);
  glUseProgram(0);
}

void Realtime::draw(GLuint shader) {
  for (const RayMarchObj &rmo : scene.getShapes()) {
    configureShapeUniforms(shader, rmo);
  }
}

void Realtime::setFBO(GLuint fbo) {
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glViewport(0, 0, scene.m_width, scene.m_height);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Realtime::initImagePlane() {
  // Four corners of Image Plane
  std::vector<GLfloat> verts = {
      -1.f, 1.f,  // top left
      -1.f, -1.f, // bottom left
      1.f,  -1.f, // bottom right
      1.f,  1.f,  // top right
      -1.f, 1.f,  // top left
      1.f,  -1.f, // bottom right
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

void Realtime::configureCameraUniforms(GLuint shader) {
  // Get all the stuff we want to use in our shader program
  glm::mat4 viewMatrix = scene.getCamera().getViewMatrix();
  glm::mat4 projMatrix = scene.getCamera().getProjMatrix();
  glm::vec4 camPosition = scene.getCamera().getCameraPosition();
  glm::mat4 invProjViewMatrix = glm::inverse(projMatrix * viewMatrix);

  // View Matrix
  GLint viewLoc = glGetUniformLocation(shader, "viewMatrix");
  glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &viewMatrix[0][0]);

  // Projection Matrix
  GLint projLoc = glGetUniformLocation(shader, "projMatrix");
  glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projMatrix[0][0]);

  // Camera Position
  GLint eyePosLoc = glGetUniformLocation(shader, "eyePosition");
  glUniform4fv(eyePosLoc, 1, &camPosition[0]);

  // Inv Proj View
  GLint invProjViewLoc = glGetUniformLocation(shader, "invProjViewMatrix");
  glUniformMatrix4fv(invProjViewLoc, 1, GL_FALSE, &invProjViewMatrix[0][0]);
}

void Realtime::configureScreenUniforms(GLuint shader) {
  glm::vec2 screenD{
      scene.m_width,
      scene.m_height,
  };
  GLuint screenDLoc = glGetUniformLocation(shader, "screenDimensions");
  glUniform2fv(screenDLoc, 1, &screenD[0]);
}

void Realtime::configureShapeUniforms(GLuint shader, const RayMarchObj &rmo) {
  // model matrix
  GLint modelMatrixID = glGetUniformLocation(shader, "modelMatrix");
  glUniformMatrix4fv(modelMatrixID, 1, GL_FALSE, &rmo.m_ctm[0][0]);
}
