#include "camera.h"
#include <iostream>

/**
 * @brief Initializes the camera from the scene json camera data
 * @param cd SceneCameraData that we are reading from
 */
void Camera::initializeCamera(SceneCameraData &cd, Settings &s) {
  // Set the basic camera params

  // Screen-related
  m_width = s.screenWidth;
  m_height = s.screenHeight;
  m_aspectRatio = m_width / static_cast<float>(m_height);

  // Frustum
  m_near = s.nearPlane;
  m_far = s.farPlane;

  // View Angle
  m_heightAngle = cd.heightAngle;
  m_viewAngleHeight = 2 * m_far * glm::tan(m_heightAngle / 2);
  m_viewAngleWidth = m_aspectRatio * m_viewAngleHeight;

  // Virtual Camera spatial info
  m_look = cd.look;
  m_pos = cd.pos;
  m_up = cd.up;

  // Set the view matrix
  setViewMatrix();
  // Set the proj matrix
  setProjMatrix();
};

/**
 * @brief Updates the camera dimension
 * @param w New width
 * @param h New height
 */
void Camera::updateCameraDimensions(int w, int h) {
  if (m_width == w && m_height == h) {
    // nop
    return;
  }
  m_width = w;
  m_height = h;
  m_aspectRatio = w / float(h);
  setProjMatrix();
}

/**
 * @brief Updates the camera-specific parameters
 * @param s Settings that is updated
 * @returns true if update was performed
 */
void Camera::updateCamera(Settings &s) {
  if (m_near == s.nearPlane && m_far == s.farPlane) {
    // If no updates are made, just return
    return;
  }
  m_near = s.nearPlane;
  m_far = s.farPlane;
  setProjMatrix();
}

/**
 * @brief Computes the view matrix of the camera given "pos", "look", and "up"
 * vectors.
 * @param pos Position of the virtual camera
 * @param look Look vector of the virtual camera
 * @param up Up vector of the virtual camera
 */
void Camera::setViewMatrix() {
  // Translation Matrix
  glm::vec4 c1(1.f, 0, 0, 0);
  glm::vec4 c2(0, 1.f, 0, 0);
  glm::vec4 c3(0, 0, 1.f, 0);
  glm::vec4 c4(-m_pos[0], -m_pos[1], -m_pos[2], 1);
  glm::mat4 translationMat(c1, c2, c3, c4);

  // Find UVW
  glm::vec3 w = -glm::normalize(m_look);
  glm::vec3 v = glm::normalize(m_up - (glm::dot(m_up, w) * w));
  glm::vec3 u = glm::cross(v, w);

  // Construct Rotation Matrix
  glm::vec4 rc1(u[0], v[0], w[0], 0);
  glm::vec4 rc2(u[1], v[1], w[1], 0);
  glm::vec4 rc3(u[2], v[2], w[2], 0);
  glm::vec4 rc4(0, 0, 0, 1.f);
  glm::mat4 rotationMat(rc1, rc2, rc3, rc4);

  // Compute the View and Inverse View Matrices
  m_view = rotationMat * translationMat;
  m_invView = glm::inverse(m_view);
}

/**
 * @brief Computes the projection matrix of the camera
 * - Get the Scale Matrix
 * - Get the unhinging Matrix
 * - Compute the projection matrix
 */
void Camera::setProjMatrix() {
  // Compute View Angle Dimensions
  m_viewAngleHeight = 2 * m_far * glm::tan(m_heightAngle / 2);
  m_viewAngleWidth = m_aspectRatio * m_viewAngleHeight;

  // Scale x, Scale y, Scale z
  float scaleX = 2 / m_viewAngleWidth;
  float scaleY = 2 / m_viewAngleHeight;
  float scaleZ = 1 / m_far;

  // Construct the scale matrix
  glm::vec4 sc1(scaleX, 0, 0, 0);
  glm::vec4 sc2(0, scaleY, 0, 0);
  glm::vec4 sc3(0, 0, scaleZ, 0);
  glm::vec4 sc4(0, 0, 0, 1.f);
  glm::mat4 scaleMatrix(sc1, sc2, sc3, sc4);

  float c = -float(m_near) / m_far;

  // Construct the unhinging matrix
  glm::vec4 c1(1.f, 0, 0, 0);
  glm::vec4 c2(0, 1.f, 0, 0);
  glm::vec4 c3(0, 0, 1.f / (1.f + c), -1.f);
  glm::vec4 c4(0, 0, -c / (1.f + c), 0);
  glm::mat4 unhingingMatrix(c1, c2, c3, c4);

  // Set the projection matrix
  m_proj = m_OpenGLRemapMatrix * unhingingMatrix * scaleMatrix;
}

/**
 * @brief Gets the View Matrix of this camera
 * @returns glm::mat4 representing camera view matrix
 */
glm::mat4 Camera::getViewMatrix() const { return m_view; }

/**
 * @brief Gets the Projection Matrix of this camera
 * @returns glm::mat4 representing camera projection matrix
 */
glm::mat4 Camera::getProjMatrix() const { return m_proj; }

/**
 * @brief Gets the Position of this camera in the world space
 * @returns glm::vec4 representing camera position
 */
glm::vec4 Camera::getCameraPosition() const { return glm::vec4(m_pos, 1); }

/**
 * @brief Gets the near plane of this camera frustum
 * @returns float representing camera near plane
 */
float Camera::getNearPlane() const { return m_near; }

/**
 * @brief Gets the far plane of this camera frustum
 * @returns float representing camera far plane
 */
float Camera::getFarPlane() const { return m_far; }

/**
 * @brief Moves the camera by the displacement and update the view matrix to
 * reflect the change
 * @param disp Amount to move
 */
void Camera::applyTranslation(glm::vec3 &disp) {
  // apply the displacement
  m_pos += disp;
  // update the view matrix
  setViewMatrix();
}

/**
 * @brief Handles the W key (move along the look vector)
 * @returns unit displacement with sensitivity applied
 */
glm::vec3 Camera::onWPressed() { return 0.35f * (100.f / m_far) * m_look; }

/**
 * @brief Handles the S key (move along the neg look vector)
 * @returns unit displacement with sensitivity applied
 */
glm::vec3 Camera::onSPressed() { return -0.35f * (100.f / m_far) * m_look; }

/**
 * @brief Handles the A key (move to the left)
 * @returns unit displacement with sensitivity applied
 */
glm::vec3 Camera::onAPressed() {
  return -0.35f * (100.f / m_far) * glm::cross(m_look, m_up);
}

/**
 * @brief Handles the D key (move to the right)
 * @returns unit displacement with sensitivity applied
 */
glm::vec3 Camera::onDPressed() {
  return 0.35f * (100.f / m_far) * glm::cross(m_look, m_up);
}

/**
 * @brief Handles the Space key (move along <0, 1, 0> in world space)
 * @returns unit displacement with sensitivity applied
 */
glm::vec3 Camera::onSpacePressed() { return glm::vec3(0, 1.f, 0); }

/**
 * @brief Handles the Control key (move along <0, -1, 0> in world space)
 * @returns unit displacement with sensitivity applied
 */
glm::vec3 Camera::onControlPressed() { return glm::vec3(0, -1.f, 0); }

/**
 * @brief Applies the rotation matrix to our look vector after which
 * we update the view matrix to reflect the change
 * @param rotationMat Rotation Matrix that you want to apply
 */
void Camera::applyRotation(glm::mat3 &rotationMat) {
  // apply the rotation matrix
  m_look = rotationMat * m_look;
  // update the view matrix
  setViewMatrix();
}

/**
 * @brief Handles the Mouse X movement which rotates the camera about
 * the axis defined by world space vector (0, 1, 0) by "angle".
 * We do a clock wise rotation
 * @param angle Amout which we want to rotate
 */
void Camera::rotateX(float deltaX) {
  float angle = 360.f * deltaX / m_width;
  float multiple = 100.f / m_far;
  angle *= multiple;
  float theta = glm::radians(angle);
  glm::mat3 rotationMat(cos(theta), 0, -1 * sin(theta), // r1
                        0, 1.f, 0,                      // r2
                        sin(theta), 0, cos(theta));     // r3
  applyRotation(rotationMat);
}

/**
 * @brief Handles the Mouse Y movement which rotates the camera about the
 * axis defined by world space vector that is perpendicular to look and up
 * vector of the camera
 */
void Camera::rotateY(float deltaY) {
  float angle = 360.f * deltaY / m_height;
  float multiple = 100.f / m_far;
  angle *= multiple / 5;
  float theta = glm::radians(angle);
  glm::vec3 axis = glm::cross(m_look, m_up);
  // Rodrigue's Formula
  float sinTheta = glm::sin(theta);
  float cosTheta = glm::cos(theta);
  glm::mat3 K(0, -axis[2], axis[1],  // r1
              axis[2], 0, -axis[0],  // r2
              -axis[1], axis[0], 0); // r3
  glm::mat3 I(1, 0, 0, 0, 1, 0, 0, 0, 1);
  glm::mat3 rotationMat = I + (sinTheta * K) + (1 - cosTheta) * (K * K);
  applyRotation(rotationMat);
}
