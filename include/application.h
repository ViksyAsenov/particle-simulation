#pragma once

#include <config.h>
#include "core/simulation.h"

enum class SimulationType
{
  NONE,
  CLOTH,
  FALLING_SAND,
};

struct SimulationTab
{
  std::string name;
  SimulationType type;
};

const SimulationTab tabs[] = {
    {"Cloth", SimulationType::CLOTH},
    {"Falling Sand", SimulationType::FALLING_SAND},
};

class Application
{
private:
  GLFWwindow *window;
  Simulation *activeSimulation;

  float zoom = 1.0f;

  unsigned int screenWidth = 1200;
  unsigned int screenHeight = 600;
  float currentUiWidth = 350.0f;

  SimulationType currentSimulation = SimulationType::NONE;

  static void cursorCallback(GLFWwindow *window, double xPosition, double yPosition);
  static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
  static void scrollCallback(GLFWwindow *window, double xOffset, double yOffset);

  static void framebufferSizeCallback(GLFWwindow *window, int width, int height);
  static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);

public:
  Application();
  ~Application();

  bool init();

  void switchSimulation(SimulationType newSimulation);

  void run();
};