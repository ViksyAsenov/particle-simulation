#pragma once

class MouseListener
{
public:
  virtual ~MouseListener() = default;

  virtual void onMouseMove(float x, float y) = 0;

  virtual void onMouseButton(int button, int action, int mods) = 0;

  virtual void onMouseScroll(float xOffset, float yOffset) = 0;

  virtual void onCameraZoom(float yOffset) {}
};