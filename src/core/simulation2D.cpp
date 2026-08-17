#include <config.h>

#include "core/simulation2D.h"
#include <glm/gtc/matrix_transform.hpp>

Simulation2D::Simulation2D(int screenWidth, int screenHeight)
{
  this->screenWidth = screenWidth;
  this->screenHeight = screenHeight;

  this->cameraPosition = glm::vec2(0.0f, 0.0f);

  setProjectionMatrix(screenWidth, screenHeight);
}

void Simulation2D::setCameraFocus(glm::vec2 focusPoint, float zoom)
{
  this->defaultCameraPosition = focusPoint;
  this->defaultCameraZoom = zoom;

  resetCamera();
}

void Simulation2D::onResize(int newScreenWidth, int newScreenHeight)
{
  if (newScreenWidth == 0 || newScreenHeight == 0)
  {
    return;
  }

  this->screenWidth = newScreenWidth;
  this->screenHeight = newScreenHeight;

  setProjectionMatrix(newScreenWidth, newScreenHeight);
}

void Simulation2D::setProjectionMatrix(int width, int height)
{
  projectionMatrix = glm::ortho(
      0.0f,
      static_cast<float>(width),
      static_cast<float>(height),
      0.0f,
      -1.0f,
      1.0f);
}

void Simulation2D::resetCamera()
{
  cameraPosition = defaultCameraPosition;
  cameraZoom = defaultCameraZoom;
}

void Simulation2D::onMouseMove(float x, float y)
{
  glm::vec2 currentMousePosition(x, y);

  if (isMiddleMouseDown)
  {
    glm::vec2 delta = currentMousePosition - lastScreenMousePosition;
    cameraPosition -= delta / cameraZoom;
  }

  lastScreenMousePosition = currentMousePosition;

  worldMousePosition.x = (x - screenWidth * 0.5f) / cameraZoom + cameraPosition.x;
  worldMousePosition.y = (y - screenHeight * 0.5f) / cameraZoom + cameraPosition.y;
}

void Simulation2D::onMouseButton(int button, int action, int mods)
{
  switch (button)
  {
  case 0:
    isLeftMouseDown = (action == 1);
    break;

  case 1:
    isRightMouseDown = (action == 1);
    break;

  case 2:
    isMiddleMouseDown = (action == 1);
    break;
  }
}

void Simulation2D::onMouseScroll(float xOffset, float yOffset)
{
  mouseScrollDeltaY = yOffset;
}

void Simulation2D::onCameraZoom(float yOffset)
{
  float zoomFactor = 1.0f + (yOffset * zoomSpeed);

  if (zoomFactor <= 0.0f)
  {
    zoomFactor = 0.1f;
  }

  cameraZoom *= zoomFactor;

  if (cameraZoom < 0.001f)
  {
    cameraZoom = 0.001f;
  }
}

glm::mat4 Simulation2D::getViewProjectionMatrix() const
{
  glm::mat4 view = glm::mat4(1.0f);

  view = glm::translate(view, glm::vec3(screenWidth * 0.5f, screenHeight * 0.5f, 0.0f));

  view = glm::scale(view, glm::vec3(cameraZoom, cameraZoom, 1.0f));

  view = glm::translate(view, glm::vec3(-cameraPosition.x, -cameraPosition.y, 0.0f));

  return projectionMatrix * view;
}