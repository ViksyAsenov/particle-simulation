#pragma once

#include <config.h>
#include "core/simulation.h"
#include "input/mouseListener.h"

class Simulation2D : public Simulation, public MouseListener
{
protected:
  glm::mat4 projectionMatrix{1.0f};
  int screenWidth;
  int screenHeight;

  glm::vec2 cameraPosition;
  float cameraZoom = 1.0f;

  glm::vec2 worldMousePosition;
  bool isLeftMouseDown = false;
  bool isRightMouseDown = false;
  bool isMiddleMouseDown = false;
  float mouseScrollY = 0.0f;

private:
  glm::vec2 lastScreenMousePosition;

public:
  Simulation2D(int screenWidth, int screenHeight);
  virtual ~Simulation2D() = default;

  virtual void onResize(int newScreenWidth, int newScreenHeight) override;
  virtual void resetCamera() override;

  virtual void onMouseMove(float x, float y) override;
  virtual void onMouseButton(int button, int action, int mods) override;
  virtual void onMouseScroll(float xOffset, float yOffset) override;
  virtual void onCameraZoom(float yOffset) override;

  glm::mat4 getViewProjectionMatrix() const;
};