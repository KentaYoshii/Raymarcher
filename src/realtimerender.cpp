#include "realtime.h"
#include "utils/ltc_matrix.h"
#include <filesystem>
#include <iostream>

// ======================== UTILITY FUNCTIONS ========================

void Realtime::setIntUniform(GLuint shader, const char *var, int val) {
  GLuint loc = glGetUniformLocation(shader, var);
  glUniform1i(loc, val);
}

void Realtime::setFloatUniform(GLuint shader, const char *var, float val) {
  GLuint loc = glGetUniformLocation(shader, var);
  glUniform1f(loc, val);
}

void Realtime::setMat4Uniform(GLuint shader, const char *var,
                              const glm::mat4 &mat) {
  GLint loc = glGetUniformLocation(shader, var);
  glUniformMatrix4fv(loc, 1, GL_FALSE, &mat[0][0]);
}

void Realtime::setVec2Uniform(GLuint shader, const char *var,
                              const glm::vec2 &v) {
  GLint loc = glGetUniformLocation(shader, var);
  glUniform2fv(loc, 1, &v[0]);
}

void Realtime::setVec3Uniform(GLuint shader, const char *var,
                              const glm::vec3 &v) {
  GLint loc = glGetUniformLocation(shader, var);
  glUniform3fv(loc, 1, &v[0]);
}

void Realtime::setVec4Uniform(GLuint shader, const char *var,
                              const glm::vec4 &v) {
  GLint loc = glGetUniformLocation(shader, var);
  glUniform4fv(loc, 1, &v[0]);
}

// ===================================================================

/**
 * @brief Performs Raymarching using our raymarch shader
 * - Set the shader
 * - Set the output FBO
 *   - If FXAA enabled, offline render first
 *   - Else just render to the wnd
 * - Set the uniforms
 * - Draws the Blank Screen
 */
void Realtime::rayMarch() {
  // Set ray march shader
  glUseProgram(m_rayMarchShader);
  // Set FBO
  if (m_enableFXAA || m_enableHDR || m_enableGammaCorrection || m_enableBloom) {
    // If FXAA, HDR, Bloom, or gamma correction enabled, render offline first
    setFBO(m_customFBO);
  } else {
    // Else go straight to application window
    setFBO(m_defaultFBO);
  }
  // Set Uniforms
  configureScreenUniforms(m_rayMarchShader);
  configureCameraUniforms(m_rayMarchShader);
  configureShapesUniforms(m_rayMarchShader);
  configureLightsUniforms(m_rayMarchShader);
  configureSettingsUniforms(m_rayMarchShader);

  // Draw
  glBindVertexArray(m_imagePlaneVAO);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  // Un-set
  glBindVertexArray(0);
  glUseProgram(0);

  // Apply HDR or gamma correction, if enabled
  if (m_enableHDR || m_enableGammaCorrection || m_enableBloom) {
    applyLightEffects();
  }

  // Apply FXAA, if enabled
  if (m_enableFXAA) {
    applyFXAA();
  }
}

/**
 * @brief Apply Gaussian Blur for Bloom lighting effect
 */
bool Realtime::applyBloom() {
  glUseProgram(m_blurShader);
  bool horizontal = true;
  for (int i = 0; i < BLOOM_BLUR_COUNT; i++) {
    glBindFramebuffer(GL_FRAMEBUFFER, m_pingpongFBO[horizontal]);
    setIntUniform(m_blurShader, "horizontal", horizontal);
    // If this is the first iteration, use the brightness texture
    // Else just used the previously blurred texture
    GLuint texToBind =
        i == 0 ? m_bloomBrightnessTexture : m_pingpongBuffer[!horizontal];
    drawToQuadWithTex(texToBind);
    horizontal = !horizontal;
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glUseProgram(0);
  return horizontal;
}

/**
 * @brief Applies HDR, Bloom, or Gamma Correction
 */
void Realtime::applyLightEffects() {
  bool side = false;
  if (m_enableBloom) {
    side = applyBloom();
  }
  glUseProgram(m_lightOptionShader);
  if (m_enableFXAA) {
    // If applying FXAA later output to default color texture
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           m_customFBOColorTexture, 0);
    glViewport(0, 0, scene.m_width, scene.m_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  } else {
    setFBO(m_defaultFBO);
  }
  // Set Uniforms
  configureLightEffectsUniforms(m_lightOptionShader, side);
  // Draw to Full Screen Quad using offline rendered hdr texture
  drawToQuadWithTex(m_hdrTexture);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glUseProgram(0);
}

/**
 * @brief Applies FXAA
 */
void Realtime::applyFXAA() {
  glUseProgram(m_fxaaShader);
  // FXAA is last processing we apply
  setFBO(m_defaultFBO);
  // Set Uniforms
  configureFXAAUniforms(m_fxaaShader);
  // Draw to Full Screen Quad using offline rendered texture
  drawToQuadWithTex(m_customFBOColorTexture);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glUseProgram(0);
}

/**
 * @brief Given tex, draw to a full screen quad
 * @param texture we want to sample from
 */
void Realtime::drawToQuadWithTex(GLuint tex) {
  // Bind full screen quad vao
  glBindVertexArray(m_fullscreenVAO);
  // Activate 0
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tex);
  // Draw
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindVertexArray(0);
}

/**
 * @brief Sets destination FBO
 * @param fbo FBO that we wish to render to
 */
void Realtime::setFBO(GLuint fbo) {
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  if (fbo == m_customFBO) {
    if (m_enableHDR || m_enableGammaCorrection || m_enableBloom) {
      // use HDR color buf to prevent clamping
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D, m_hdrTexture, 0);
    } else {
      // use default color buffer
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D, m_customFBOColorTexture, 0);
    }
  }
  glViewport(0, 0, scene.m_width, scene.m_height);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

/**
 * @brief Initializes the [-1,1] blank screen vao/vbo pairing to be used for
 * raymarching
 */
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

/**
 * @brief Initializes full screen quad to be used to apply post processing
 * effects
 */
void Realtime::initFullScreenQuad() {
  // Construct a Fullscreen Quads (projector screen)
  std::vector<GLfloat> fullscreen_quad_data = {
      -1.f, 1.f,  0.0f, // top left
      0.f,  1.f,        // tl uv
      -1.f, -1.f, 0.0f, // bottom left
      0.f,  0.f,        // bl uv
      1.f,  -1.f, 0.0f, // bottom right
      1.f,  0.f,        // br uv
      1.f,  1.f,  0.0f, // top right
      1.f,  1.f,        // tr uv
      -1.f, 1.f,  0.0f, // top left
      0.f,  1.f,        // tl uv
      1.f,  -1.f, 0.0f, // bottom right
      1.f,  0.f,        // br uv
  };

  // Generate and bind a VBO and VAO for a fullscreen quad

  // - VBO
  glGenBuffers(1, &m_fullscreenVBO);
  glBindBuffer(GL_ARRAY_BUFFER, m_fullscreenVBO);
  glBufferData(GL_ARRAY_BUFFER, fullscreen_quad_data.size() * sizeof(GLfloat),
               fullscreen_quad_data.data(), GL_STATIC_DRAW);
  // - VAO
  glGenVertexArrays(1, &m_fullscreenVAO);
  glBindVertexArray(m_fullscreenVAO);
  // pos
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), nullptr);
  // uv
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
                        reinterpret_cast<void *>(3 * sizeof(GLfloat)));

  // unbind the fullscreen quad's VBO and VAO
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

/**
 * @brief Creates the material texture object for each shape
 * Invoke once when the scene is first loaded
 */
void Realtime::initShapesTextures() {
  std::unordered_map<std::string, GLuint> texMap;
  // Set the texture IDs for the shapes that use them
  for (RayMarchObj &rts : scene.getShapes()) {
    if (!rts.m_material.textureMap.isUsed) {
      continue;
    }
    std::string texName = rts.m_material.textureMap.filename;
    if (texMap.find(texName) == texMap.end()) {
      // not found yet -> generate
      glGenTextures(1, &rts.m_texture);
      texMap[texName] = rts.m_texture;
    } else {
      // found a texture id -> reuse
      rts.m_texture = texMap[texName];
    }
  }

  // For destruction later
  m_TextureMap = texMap;

  // For each texture, initialize
  std::map<std::string, TextureInfo> sceneTexs = scene.getShapesTextures();
  int cnt = 0;
  for (auto const &[name, id] : texMap) {
    TextureInfo texInfo = sceneTexs[name];
    glActiveTexture(GL_TEXTURE0 + cnt);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texInfo.width, texInfo.height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, texInfo.image.bits());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    cnt++;
  }
}

/**
 * @brief Initializes default variables
 */
void Realtime::initDefaults() {
  // Default material texture
  // - for shapes that don't have textures associated with them
  //    - if we don't do this GLSL complains
  glActiveTexture(GL_TEXTURE0);
  glGenTextures(1, &m_defaultShapeTexture);
  glBindTexture(GL_TEXTURE_2D, m_defaultShapeTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               std::vector<GLfloat>{0, 0}.data());
  glBindTexture(GL_TEXTURE_2D, 0);
  // NULL CUBE MAP TEXTURE
  glActiveTexture(GL_TEXTURE0 + SKYBOX_TEX_UNIT_OFF);
  glGenTextures(1, &m_nullCubeMapTexture);
  glBindTexture(GL_TEXTURE_CUBE_MAP, m_nullCubeMapTexture);
  for (int i = 0; i < 6; i++) {
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, 1, 1, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, std::vector<GLfloat>{0, 0}.data());
  }
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
  // NULL Bloom Texture
  glGenTextures(1, &m_nullBloomBlurTexture);
  glBindTexture(GL_TEXTURE_2D, m_nullBloomBlurTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, scene.m_width, scene.m_height, 0,
               GL_RGBA, GL_FLOAT, std::vector<GLfloat>{0, 0}.data());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);
}

/**
 * @brief Initializes the shader with constant uniforms
 */
void Realtime::initShader() {
  // Raymarch shader
  glUseProgram(m_rayMarchShader);
  // Set the textures to use correct slots
  GLuint texsLoc = glGetUniformLocation(m_rayMarchShader, "objTextures");
  for (int i = 0; i < MAX_NUM_TEXTURES; i++) {
    // Bind to default
    glActiveTexture(GL_TEXTURE0 + i);
    glBindTexture(GL_TEXTURE_2D, m_defaultShapeTexture);
    glUniform1i(texsLoc + i, i);
  }
  // Set the skybox tex unit to the next available
  setIntUniform(m_rayMarchShader, "skybox", SKYBOX_TEX_UNIT_OFF);
  // Set the M and LTU texture units for area lights
  setIntUniform(m_rayMarchShader, "LTC1", LTC1_TEX_UNIT_OFF);
  setIntUniform(m_rayMarchShader, "LTC2", LTC2_TEX_UNIT_OFF);
  // Bind the textures
  glActiveTexture(GL_TEXTURE0 + LTC1_TEX_UNIT_OFF);
  glBindTexture(GL_TEXTURE_2D, m_mTexture);
  glActiveTexture(GL_TEXTURE0 + LTC2_TEX_UNIT_OFF);
  glBindTexture(GL_TEXTURE_2D, m_ltuTexture);
  glUseProgram(0);

  // FXAA Shader (fxaa)
  glUseProgram(m_fxaaShader);
  setIntUniform(m_fxaaShader, "screenTexture", 0);
  glUseProgram(0);

  // Display Option Shader (gamma correct / HDR / Bloom)
  glUseProgram(m_lightOptionShader);
  setIntUniform(m_lightOptionShader, "hdrBuffer", 0);
  setIntUniform(m_lightOptionShader, "bloomBlur", 1);
  glUseProgram(0);

  // Debugging Shader
  glUseProgram(m_debugShader);
  setIntUniform(m_debugShader, "debugTexture", 0);
  glUseProgram(0);

  // Bloom Blur Shader
  glUseProgram(m_blurShader);
  setIntUniform(m_blurShader, "image", 0);
  glUseProgram(0);
}

/**
 * @brief Initializes the custom FBO for offline rendering
 */
void Realtime::initCustomFBO() {
  // ColorBuffer
  glGenTextures(1, &m_customFBOColorTexture);
  glBindTexture(GL_TEXTURE_2D, m_customFBOColorTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, scene.m_width, scene.m_height, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);

  // HDR ColorBuffer
  glGenTextures(1, &m_hdrTexture);
  glBindTexture(GL_TEXTURE_2D, m_hdrTexture);
  // - note the RGBA16F internal format
  // - this will prevent from frag shader clamping color val to [0, 1] range
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, scene.m_width, scene.m_height, 0,
               GL_RGBA, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);

  // Bloom BrightColorBuffer
  glGenTextures(1, &m_bloomBrightnessTexture);
  glBindTexture(GL_TEXTURE_2D, m_bloomBrightnessTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, scene.m_width, scene.m_height, 0,
               GL_RGBA, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);

  // RenderBuffer
  glGenRenderbuffers(1, &m_customFBORenderBuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, m_customFBORenderBuffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, scene.m_width,
                        scene.m_height);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);

  // FBO
  glGenFramebuffers(1, &m_customFBO);
  glBindFramebuffer(GL_FRAMEBUFFER, m_customFBO);
  // - set normal color buffer as default 0
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         m_customFBOColorTexture, 0);
  // - set brightness as default 1
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                         m_bloomBrightnessTexture, 0);
  GLuint attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
  glDrawBuffers(2, attachments);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, m_customFBORenderBuffer);
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    std::cout << "Custom Buffer Incomplete" << std::endl;
  }

  // =================== Bloom ========================
  // - two fbos for horizontal and vertical dir
  glGenFramebuffers(2, m_pingpongFBO);
  glGenTextures(2, m_pingpongBuffer);
  for (GLuint i = 0; i < 2; i++) {
    glBindFramebuffer(GL_FRAMEBUFFER, m_pingpongFBO[i]);
    glBindTexture(GL_TEXTURE_2D, m_pingpongBuffer[i]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, scene.m_width, scene.m_height, 0,
                 GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           m_pingpongBuffer[i], 0);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);
}

/**
 * @brief Sets the cube map texture
 */
void Realtime::initCubeMap(CUBEMAP type) {
  if (type == CUBEMAP::UNUSED)
    return;
  std::filesystem::path basepath =
      std::filesystem::path(settings.sceneFilePath).parent_path().parent_path();
  // Get the image paths
  std::vector<std::string> faces = scene.getCubeMapWithType(type);
  glGenTextures(1, &m_cubeMapTexture);
  glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubeMapTexture);
  int width, height;
  for (int i = 0; i < faces.size(); i++) {
    // Load up each face
    QImage myImage;
    std::filesystem::path fileRelativePath(faces[i]);
    QString str((basepath / fileRelativePath).string().data());
    if (!myImage.load(str)) {
      std::cout << "Failed to load in image" << std::endl;
      return;
    }

    myImage = myImage.convertToFormat(QImage::Format_RGBA8888).mirrored();

    width = myImage.width();
    height = myImage.height();
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, width, height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, myImage.bits());
  }
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

/**
 * @brief Sets the uniforms that are related to camera/eye
 * @param shader Shader program we are using
 */
void Realtime::configureCameraUniforms(GLuint shader) {
  // Get all the stuff we want to use in our shader program
  glm::mat4 viewMatrix = scene.getCamera().getViewMatrix();
  glm::mat4 projMatrix = scene.getCamera().getProjMatrix();
  glm::vec4 camPosition = scene.getCamera().getCameraPosition();
  glm::mat4 invProjViewMatrix = glm::inverse(projMatrix * viewMatrix);
  float near = scene.getCamera().getNearPlane();
  float far = scene.getCamera().getFarPlane();
  // Near & Far
  setFloatUniform(shader, "near", near);
  setFloatUniform(shader, "far", far);
  // View Matrix
  setMat4Uniform(shader, "viewMatrix", viewMatrix);
  // Projection Matrix
  setMat4Uniform(shader, "projMatrix", projMatrix);
  // Camera Position
  setVec4Uniform(shader, "eyePosition", camPosition);
  // Inv Proj View
  setMat4Uniform(shader, "invProjViewMatrix", invProjViewMatrix);
}

/**
 * @brief Sets the uniforms that are related to current screen
 * @param shader Shader program we are using
 */
void Realtime::configureScreenUniforms(GLuint shader) {
  glm::vec2 screenD{
      scene.m_width,
      scene.m_height,
  };
  // Screen Dimensions
  setVec2Uniform(shader, "screenDimensions", screenD);
  // ITime
  setFloatUniform(shader, "iTime", m_delta);
  // Sky Box
  glActiveTexture(GL_TEXTURE0 + SKYBOX_TEX_UNIT_OFF);
  if (m_idxSkyBox) {
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubeMapTexture);
  } else {
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_nullCubeMapTexture);
  }
}

/**
 * @brief Sets the uniforms for all the scene lights
 * @param shader Shader program we are using
 */
void Realtime::configureLightsUniforms(GLuint shader) {
  int cnt = 0;
  // ka (ambience)
  setFloatUniform(shader, "ka", scene.getGlobalData().ka);
  // kd (diffuse)
  setFloatUniform(shader, "kd", scene.getGlobalData().kd);
  // ks (specular)
  setFloatUniform(shader, "ks", scene.getGlobalData().ks);
  // kt (transparent)
  setFloatUniform(shader, "kt", scene.getGlobalData().kt);
  for (const SceneLightData &light : scene.getLights()) {
    if (cnt == MAX_NUM_LIGHTS) {
      return;
    }
    std::string base = "lights[" + std::to_string(cnt) + "].";
    // Type
    setIntUniform(shader, (base + "type").c_str(),
                  static_cast<std::underlying_type_t<LightType>>(light.type));
    // Pos
    setVec3Uniform(shader, (base + "lightPos").c_str(), light.pos);
    // Dir
    setVec3Uniform(shader, (base + "lightDir").c_str(), light.dir);
    // Color
    setVec3Uniform(shader, (base + "lightColor").c_str(), light.color);
    // Attenuation
    setVec3Uniform(shader, (base + "lightFunc").c_str(), light.function);
    // Angle
    setFloatUniform(shader, (base + "lightAngle").c_str(), light.angle);
    // Penumbra
    setFloatUniform(shader, (base + "lightPenumbra").c_str(), light.penumbra);
    // Area Light Uniforms
    if (light.type == LightType::LIGHT_AREA) {
      // Area Light Intensity
      setFloatUniform(shader, (base + "intensity").c_str(), light.intensity);
      // Area Light Two-Sided ness
      setIntUniform(shader, (base + "twoSided").c_str(), true);
      // Rectangle Area Light Position
      for (int i = 0; i < 4; i++) {
        auto currCorner = glm::mat4(light.ctm) * glm::vec4(corners[i], 1.f);
        setVec3Uniform(shader,
                       (base + "points[" + std::to_string(i) + "]").c_str(),
                       currCorner);
      }
    }
    cnt += 1;
  }
  // Number of lights
  setIntUniform(shader, "numLights", cnt);

  glActiveTexture(GL_TEXTURE0 + LTC1_TEX_UNIT_OFF);
  glBindTexture(GL_TEXTURE_2D, m_mTexture);
  glActiveTexture(GL_TEXTURE0 + LTC2_TEX_UNIT_OFF);
  glBindTexture(GL_TEXTURE_2D, m_ltuTexture);
}

/**
 * @brief Sets all the uniforms for all the rendering options that are available
 * @param shader Shader program we are using
 */
void Realtime::configureSettingsUniforms(GLuint shader) {
  // Soft Shadow
  setIntUniform(shader, "enableSoftShadow", m_enableSoftShadow);
  // Reflection
  setIntUniform(shader, "enableReflection", m_enableReflection);
  // Refraction
  setIntUniform(shader, "enableRefraction", m_enableRefraction);
  // Ambient Occulusion
  setIntUniform(shader, "enableAmbientOcculusion", m_enableAmbientOcclusion);
  // Sky Box
  setIntUniform(shader, "enableSkyBox", m_idxSkyBox);
}

/**
 * @brief Sets all the uniforms for all the shapes in our scene
 * @param shader Shader program we are using
 */
void Realtime::configureShapesUniforms(GLuint shader) {
  int cnt = 0;
  int texCnt = 0;
  std::map<std::string, int> texMap;
  for (const RayMarchObj &obj : scene.getShapes()) {
    if (cnt == MAX_NUM_SHAPES) {
      return;
    }
    std::string base = "objects[" + std::to_string(cnt) + "].";
    // Type
    setIntUniform(
        shader, (base + "type").c_str(),
        static_cast<std::underlying_type_t<PrimitiveType>>(obj.m_type));
    // Inverse model matrix
    setMat4Uniform(shader, (base + "invModelMatrix").c_str(), obj.m_ctmInv);
    // Scale Factor
    // - need this to undo the side-effect of non-rigid tranform
    float scaleF =
        fmin(obj.m_scale[0][0], fmin(obj.m_scale[1][1], obj.m_scale[2][2]));
    setFloatUniform(shader, (base + "scaleFactor").c_str(), scaleF);
    // shininess
    setFloatUniform(shader, (base + "shininess").c_str(),
                    obj.m_material.shininess);
    // cAmbient
    setVec3Uniform(shader, (base + "cAmbient").c_str(),
                   obj.m_material.cAmbient);
    // cDiffuse
    setVec3Uniform(shader, (base + "cDiffuse").c_str(),
                   obj.m_material.cDiffuse);
    // cSpecular
    setVec3Uniform(shader, (base + "cSpecular").c_str(),
                   obj.m_material.cSpecular);
    // cReflective
    setVec3Uniform(shader, (base + "cReflective").c_str(),
                   obj.m_material.cReflective);
    // cTransparent
    setVec3Uniform(shader, (base + "cTransparent").c_str(),
                   obj.m_material.cTransparent);
    // blend
    setFloatUniform(shader, (base + "blend").c_str(), obj.m_material.blend);
    // ior
    setFloatUniform(shader, (base + "ior").c_str(), obj.m_material.ior);
    // repeatU
    setFloatUniform(shader, (base + "repeatU").c_str(),
                    obj.m_material.textureMap.repeatU);
    // repeatV
    setFloatUniform(shader, (base + "repeatV").c_str(),
                    obj.m_material.textureMap.repeatV);
    // isEmissive
    setIntUniform(shader, (base + "isEmissive").c_str(), obj.m_isEmissive);
    // color
    setVec3Uniform(shader, (base + "color").c_str(), obj.m_color);
    // lightidx
    setIntUniform(shader, (base + "lightIdx").c_str(), obj.m_lightIdx);

    cnt++;

    if (obj.m_texture == -1) {
      // if texture not used, set to -1
      setIntUniform(shader, (base + "texLoc").c_str(), -1);
      continue;
    }

    std::string texName = obj.m_material.textureMap.filename;

    if (texMap.find(texName) == texMap.end() && texCnt != MAX_NUM_TEXTURES) {
      // If texture not bound yet and we have not reached the limit
      glActiveTexture(GL_TEXTURE0 + texCnt);
      glBindTexture(GL_TEXTURE_2D, obj.m_texture);
      // "texName" is bound to unit 0 + "texCnt"
      texMap[texName] = texCnt;
      texCnt++;
      glActiveTexture(0);
    }
    setIntUniform(shader, (base + "texLoc").c_str(), texMap[texName]);
  }
  setIntUniform(shader, "numObjects", cnt);
}

/**
 * @brief Initializes fxaa uniforms
 */
void Realtime::configureFXAAUniforms(GLuint shader) {
  float inverseWidth = 1.0 / scene.m_width;
  float inverseHeight = 1.0 / scene.m_height;
  glm::vec2 inverseScreen{inverseWidth, inverseHeight};
  // Inverse Screen Dimensions
  setVec2Uniform(shader, "inverseScreenSize", inverseScreen);
}

/**
 * @brief Initializes light effect uniforms
 */
void Realtime::configureLightEffectsUniforms(GLuint shader, bool side) {
  // Exposure
  setFloatUniform(shader, "exposure", m_exposure);
  // HDR enable
  setIntUniform(shader, "hdr", m_enableHDR);
  // Bloom enable
  setIntUniform(shader, "bloom", m_enableBloom);
  glActiveTexture(GL_TEXTURE1);
  if (m_enableBloom) {
    glBindTexture(GL_TEXTURE_2D, m_pingpongBuffer[side]);
  } else {
    glBindTexture(GL_TEXTURE_2D, m_nullBloomBlurTexture);
  }
  glActiveTexture(0);
}

/**
 * @brief Destroyes all generated shape material texture
 */
void Realtime::destroyShapesTextures() {
  for (auto &[name, id] : m_TextureMap) {
    glDeleteTextures(1, &id);
  }
  m_TextureMap.clear();
}

/**
 * @brief Clean up any rss allocated for our custom FBO
 */
void Realtime::destroyCustomFBO() {
  glDeleteTextures(1, &m_hdrTexture);
  glDeleteTextures(1, &m_bloomBrightnessTexture);
  glDeleteTextures(1, &m_customFBOColorTexture);
  glDeleteTextures(2, m_pingpongBuffer);
  glDeleteRenderbuffers(1, &m_customFBORenderBuffer);
  glDeleteFramebuffers(1, &m_customFBO);
  glDeleteFramebuffers(2, m_pingpongFBO);
}

/**
 * @brief Update the settings
 */
void Realtime::updateUISettings() {
  // Update the options
  m_exposure = settings.exposure;
  m_enableGammaCorrection = settings.enableGammaCorrection;
  m_enableHDR = settings.enableHDR;
  m_enableBloom = settings.enableBloom;
  m_enableSoftShadow = settings.enableSoftShadow;
  m_enableReflection = settings.enableReflection;
  m_enableRefraction = settings.enableRefraction;
  m_enableAmbientOcclusion = settings.enableAmbientOcculusion;
  m_enableFXAA = settings.enableFXAA;
  if (m_idxSkyBox != settings.idxSkyBox) {
    // If new sky box is selected
    if (m_idxSkyBox) {
      // If a cube map was already loaded
      glDeleteTextures(1, &m_cubeMapTexture);
    }
    // Create the new cube map for selected skybox
    initCubeMap(static_cast<CUBEMAP>(settings.idxSkyBox));
  }
  m_idxSkyBox = settings.idxSkyBox;
}

// =============== AREA Lights ==============
// Most of this is lowk black magic
// source: https://learnopengl.com/Guest-Articles/2022/Area-Lights

/**
 * @brief Load M Texture
 */
void Realtime::loadMTexture() {
  glGenTextures(1, &m_mTexture);
  glBindTexture(GL_TEXTURE_2D, m_mTexture);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_FLOAT, LTC1);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glBindTexture(GL_TEXTURE_2D, 0);
}

/**
 * @brief Load LTU Texture
 */
void Realtime::loadLTUTexture() {
  glGenTextures(1, &m_ltuTexture);
  glBindTexture(GL_TEXTURE_2D, m_ltuTexture);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_FLOAT, LTC2);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glBindTexture(GL_TEXTURE_2D, 0);
}
