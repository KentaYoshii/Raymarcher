#include "realtime.h"
#include <iostream>

void Realtime::rayMarch() {
  // Set ray march shader
  glUseProgram(m_rayMarchShader);
  // Set FBO
  setFBO(m_defaultFBO);
  // Set Uniforms
  configureScreenUniforms(m_rayMarchShader);
  configureCameraUniforms(m_rayMarchShader);
  configureShapesUniforms(m_rayMarchShader);
  configureLightsUniforms(m_rayMarchShader);
  // Draw
  glBindVertexArray(m_imagePlaneVAO);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  // Un-set
  glBindVertexArray(0);
  glUseProgram(0);
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
  float near = scene.getCamera().getNearPlane();
  float far = scene.getCamera().getFarPlane();

  // Near & Far
  GLint nearLoc = glGetUniformLocation(shader, "near");
  glUniform1f(nearLoc, near);
  GLint farLoc = glGetUniformLocation(shader, "far");
  glUniform1f(farLoc, far);

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

void Realtime::configureLightsUniforms(GLuint shader) {
  // ka (ambience)
  GLint kaLoc = glGetUniformLocation(shader, "ka");
  glUniform1f(kaLoc, scene.getGlobalData().ka);

  // kd (diffuse)
  GLint kdLoc = glGetUniformLocation(shader, "kd");
  glUniform1f(kdLoc, scene.getGlobalData().kd);

  // ks (specular)
  GLint ksLoc = glGetUniformLocation(shader, "ks");
  glUniform1f(ksLoc, scene.getGlobalData().ks);

  int cnt = 0;
  for (const SceneLightData &light : scene.getLights()) {
    if (cnt == 10) {
      return;
    }
    // Type
    GLuint typeLoc = glGetUniformLocation(
        shader, ("lights[" + std::to_string(cnt) + "].type").c_str());
    glUniform1i(typeLoc,
                static_cast<std::underlying_type_t<LightType>>(light.type));
    // Pos
    GLuint posLoc = glGetUniformLocation(
        shader, ("lights[" + std::to_string(cnt) + "].lightPos").c_str());
    glUniform3fv(posLoc, 1, &light.pos[0]);
    // Dir
    GLuint dirLoc = glGetUniformLocation(
        shader, ("lights[" + std::to_string(cnt) + "].lightDir").c_str());
    glUniform3fv(dirLoc, 1, &light.dir[0]);
    // Color
    GLuint colorLoc = glGetUniformLocation(
        shader, ("lights[" + std::to_string(cnt) + "].lightColor").c_str());
    glUniform3fv(colorLoc, 1, &light.color[0]);
    // Attenuation
    GLuint funcLoc = glGetUniformLocation(
        shader, ("lights[" + std::to_string(cnt) + "].lightFunc").c_str());
    glUniform3fv(funcLoc, 1, &light.function[0]);
    // Angle
    GLuint angleLoc = glGetUniformLocation(
        shader, ("lights[" + std::to_string(cnt) + "].lightAngle").c_str());
    glUniform1f(angleLoc, light.angle);
    // Penumbra
    GLuint penumbraLoc = glGetUniformLocation(
        shader, ("lights[" + std::to_string(cnt) + "].lightPenumbra").c_str());
    glUniform1f(penumbraLoc, light.penumbra);
    cnt += 1;
  }
  // Finally, record the number of lights
  GLint numLightsLoc = glGetUniformLocation(shader, "numLights");
  glUniform1i(numLightsLoc, cnt);
}

void Realtime::configureShapesUniforms(GLuint shader) {
  int cnt = 0;
  for (const RayMarchObj &obj : scene.getShapes()) {
    if (cnt == 50) {
      return;
    }
    // Type
    GLuint typeLoc = glGetUniformLocation(
        shader, ("objects[" + std::to_string(cnt) + "].type").c_str());
    glUniform1i(typeLoc,
                static_cast<std::underlying_type_t<PrimitiveType>>(obj.m_type));
    // Inverse model matrix
    GLuint invModelLoc = glGetUniformLocation(
        shader,
        ("objects[" + std::to_string(cnt) + "].invModelMatrix").c_str());
    glUniformMatrix4fv(invModelLoc, 1, GL_FALSE, &obj.m_ctmInv[0][0]);
    // Scale Factor
    float scaleF =
        fmin(obj.m_scale[0][0], fmin(obj.m_scale[1][1], obj.m_scale[2][2]));
    GLuint scaleLoc = glGetUniformLocation(
        shader, ("objects[" + std::to_string(cnt) + "].scaleFactor").c_str());
    glUniform1f(scaleLoc, scaleF);

    // Material Property

    // shininess
    GLint shininessLoc = glGetUniformLocation(
        shader, ("objects[" + std::to_string(cnt) + "].shininess").c_str());
    glUniform1f(shininessLoc, obj.m_material.shininess);

    // cAmbient
    GLint cAmbientLoc = glGetUniformLocation(
        shader, ("objects[" + std::to_string(cnt) + "].cAmbient").c_str());
    glUniform3fv(cAmbientLoc, 1, &obj.m_material.cAmbient[0]);

    // cDiffuse
    GLint cDiffuseLoc = glGetUniformLocation(
        shader, ("objects[" + std::to_string(cnt) + "].cDiffuse").c_str());
    glUniform3fv(cDiffuseLoc, 1, &obj.m_material.cDiffuse[0]);

    // cSpecular
    GLint cSpecularLoc = glGetUniformLocation(
        shader, ("objects[" + std::to_string(cnt) + "].cSpecular").c_str());
    glUniform3fv(cSpecularLoc, 1, &obj.m_material.cSpecular[0]);
    cnt++;
  }
  // num objs
  GLuint numLoc = glGetUniformLocation(shader, "numObjects");
  glUniform1i(numLoc, cnt);
}
