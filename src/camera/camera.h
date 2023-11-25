#ifndef CAMERA_H
#define CAMERA_H

#include "settings.h"
#include "utils/scenedata.h"

struct Camera {
  // Struct that represents scene camera

public:
  // PUBLIC METHODS

  // Sets the camera up with the new scene
  void initializeCamera(SceneCameraData &cd, Settings &s);
  // Updates the camera with new far and near plane values
  void updateCamera(Settings &s);
  // Updates the camera with new screen dimensions
  void updateCameraDimensions(int newWidth, int newHeight);
  // Gets near plane
  float getNearPlane() const;
  // Gets far plane
  float getFarPlane() const;
  // Gets the View Matrix of the camera
  glm::mat4 getViewMatrix() const;
  // Gets the Projection Matrix of the camera
  glm::mat4 getProjMatrix() const;
  // Gets the Camera Position in the world space
  glm::vec4 getCameraPosition() const;

  // Translation

  // Applies Accumulated Translation
  void applyTranslation(glm::vec3 &disp);
  // Moves in dir of look vector
  glm::vec3 onWPressed();
  // Moves in oposite dir of look
  glm::vec3 onSPressed();
  // Moves to the left, perpendicular to look and up
  glm::vec3 onAPressed();
  // Moves to the right, perpendicular to look and up
  glm::vec3 onDPressed();
  // Moves along world space (0, 1, 0)
  glm::vec3 onSpacePressed();
  // Moves along world space (0, -1, 0)
  glm::vec3 onControlPressed();

  // Rotation

  // Applies Rotation
  void applyRotation(glm::mat3 &mat);
  // Rotates the Camera about the axis defined by world space vector (0, 1, 0)
  void rotateX(float deltaX);
  // Rotates the Camera about the axis defined by world space vector that is
  // perpendicular to look and pos
  void rotateY(float deltaY);

private:
  // PRIVATE METHODS

  // Sets the Projection Matrix of the camera
  void setProjMatrix();
  // Sets the View Matrix and World space position of the camera
  void setViewMatrix();

private:
  // PRIVATE MEMBERS

  // View Angle
  float m_viewAngleWidth;
  float m_viewAngleHeight;
  float m_heightAngle;

  // Virtual Camera
  glm::vec3 m_look;
  glm::vec3 m_pos;
  glm::vec3 m_up;

  // Camera View/Inv View Matrices
  glm::mat4 m_view;
  glm::mat4 m_invView;

  // Camera Projection Matrix
  glm::mat4 m_proj;
  // Used to remap z values to OpenGL standard
  glm::mat4 m_OpenGLRemapMatrix{
      1.f, 0,   0,    0,   // c1
      0,   1.f, 0,    0,   // c2
      0,   0,   -2.f, 0.f, // c3
      0,   0,   -1.f, 1.f, // c4
  };

  // Screen Dimension/Ratio
  int m_width;
  int m_height;
  float m_aspectRatio;

  // Camera Frustum far plane
  float m_near;
  float m_far = 1.f;
};

#endif // CAMERA_H
