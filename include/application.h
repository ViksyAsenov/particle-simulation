#pragma once

#include <config.h>
#include "core/simulation.h"

constexpr int SCREEN_WIDTH = 800;
constexpr int SCREEN_HEIGHT = 600;

class Application
{
private:
  GLFWwindow *window;
  Simulation *activeSimulation;
  float zoom = 1.0f;

  static void cursorCallback(GLFWwindow *window, double xPosition, double yPosition);
  static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
  static void scrollCallback(GLFWwindow *window, double xOffset, double yOffset);

  static void framebufferSizeCallback(GLFWwindow *window, int width, int height);
  static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);

  void syncSimulationResolution();

public:
  Application();
  ~Application();

  bool init();
  void run();
};