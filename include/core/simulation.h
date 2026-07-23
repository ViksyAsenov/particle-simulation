#pragma once

class Simulation
{
public:
  virtual ~Simulation() = default;

  virtual void init() = 0;

  virtual void update(float deltaTime) = 0;

  virtual void render() = 0;

  virtual void renderUI() = 0;

  virtual void onResize(int newScreenWidth, int newScreenHeight) {};
  virtual void resetCamera() {};
};