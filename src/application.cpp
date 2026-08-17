#include <config.h>
#include "application.h"
#include "simulations/clothSimulation.h"
#include "simulations/fallingSandSimulation.h"
#include "simulations/slimeMoldSimulation.h"
#include "simulations/nBodyGravitySimulation.h"
#include "input/mouseListener.h"

Application::Application()
{
  this->window = nullptr;
  this->activeSimulation = nullptr;
}

Application::~Application()
{
  if (activeSimulation)
  {
    delete activeSimulation;
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  if (window)
  {
    glfwDestroyWindow(window);
  }

  glfwTerminate();
}

bool Application::init()
{
  if (!glfwInit())
  {
    std::cerr << "Failed to initialize GLFW" << std::endl;

    return false;
  }

  glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
  window = glfwCreateWindow(screenWidth, screenHeight, "Particle Simulation", NULL, NULL);

  if (window == NULL)
  {
    glfwTerminate();

    std::cerr << "Failed to create GLFW window" << std::endl;

    return false;
  }

  glfwMakeContextCurrent(window);

  // Disables VSync
  glfwSwapInterval(0);

  if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
  {
    glfwTerminate();

    std::cerr << "Failed to initialize GLAD" << std::endl;

    return false;
  }

  glViewport(static_cast<int>(currentUiWidth), 0, screenWidth - static_cast<int>(currentUiWidth), screenHeight);

  glfwSetWindowUserPointer(window, this);
  glfwSetCursorPosCallback(window, cursorCallback);
  glfwSetMouseButtonCallback(window, mouseButtonCallback);
  glfwSetScrollCallback(window, scrollCallback);
  glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
  glfwSetKeyCallback(window, keyCallback);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();

  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 460 core");
  glClearColor(0.0f, 0.0f, 0.0f, 0.75f);

  return true;
}

void Application::run()
{
  double accumulator = 0.0;
  const double fixedTimeStep = 1.0 / 60.0;

  double lastFrameTime = glfwGetTime();

  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();

    double currentFrameTime = glfwGetTime();
    float deltaTime = static_cast<float>(currentFrameTime - lastFrameTime);
    lastFrameTime = currentFrameTime;

    if (deltaTime > 0.25f)
    {
      deltaTime = 0.25f; // Cap deltaTime to avoid spiral of death
    }

    accumulator += deltaTime;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);

    ImGui::SetNextWindowSize(ImVec2(currentUiWidth, viewport->Size.y));

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

    ImGui::Begin("Simulation Controls", nullptr, windowFlags);

    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Frame time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);

    ImGui::Separator();

    if (ImGui::BeginTabBar("SimulationTabs"))
    {
      for (const auto &tab : tabs)
      {
        if (ImGui::BeginTabItem(tab.name.c_str()))
        {
          if (currentSimulation != tab.type)
          {
            switchSimulation(tab.type);
            currentSimulation = tab.type;
          }

          if (activeSimulation)
          {
            activeSimulation->renderUI();
          }

          ImGui::EndTabItem();
        }
      }

      ImGui::EndTabBar();
    }

    ImGui::End();

    if (activeSimulation)
    {
      while (accumulator >= fixedTimeStep)
      {
        activeSimulation->update(fixedTimeStep);
        accumulator -= fixedTimeStep;
      }
    }

    if (activeSimulation && !activeSimulation->handlesOwnClear())
    {
      glClear(GL_COLOR_BUFFER_BIT);
    }

    if (activeSimulation)
    {
      activeSimulation->render();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
  }
}

void Application::switchSimulation(SimulationType newSimulation)
{
  if (activeSimulation != nullptr)
  {
    delete activeSimulation;
    activeSimulation = nullptr;
  }

  float simulationWidth = screenWidth - currentUiWidth;

  switch (newSimulation)
  {
  case SimulationType::CLOTH:
    activeSimulation = new ClothSimulation(40, 30, 15.0f, simulationWidth, screenHeight);
    break;
  case SimulationType::FALLING_SAND:
    activeSimulation = new FallingSandSimulation(40, 30, 15.0f, simulationWidth, screenHeight);
    break;
  case SimulationType::SLIME_MOLD:
    activeSimulation = new SlimeMoldSimulation(350000, simulationWidth, screenHeight);
    break;
  case SimulationType::N_BODY_GRAVITY:
    activeSimulation = new NBodyGravitySimulation(simulationWidth, screenHeight);
    break;
  }

  if (activeSimulation)
  {
    activeSimulation->init();
  }
}

void Application::cursorCallback(GLFWwindow *window, double xPosition, double yPosition)
{
  if (ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse)
  {
    return;
  }

  Application *app = static_cast<Application *>(glfwGetWindowUserPointer(window));

  if (!app || !app->activeSimulation)
  {
    return;
  }

  MouseListener *listener = dynamic_cast<MouseListener *>(app->activeSimulation);
  if (!listener)
  {
    return;
  }

  float adjustedX = static_cast<float>(xPosition) - app->currentUiWidth;

  listener->onMouseMove(adjustedX, static_cast<float>(yPosition));
}

void Application::mouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
  if (ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse)
  {
    return;
  }

  Application *app = static_cast<Application *>(glfwGetWindowUserPointer(window));

  if (!app || !app->activeSimulation)
  {
    return;
  }

  MouseListener *listener = dynamic_cast<MouseListener *>(app->activeSimulation);
  if (!listener)
  {
    return;
  }

  listener->onMouseButton(button, action, mods);
}

void Application::scrollCallback(GLFWwindow *window, double xOffset, double yOffset)
{
  if (ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse)
  {
    return;
  }

  Application *app = static_cast<Application *>(glfwGetWindowUserPointer(window));

  if (!app || !app->activeSimulation)
  {
    return;
  }

  MouseListener *listener = dynamic_cast<MouseListener *>(app->activeSimulation);
  if (!listener)
  {
    return;
  }

  bool isCtrlDown = (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS);

  if (isCtrlDown)
  {
    listener->onCameraZoom(static_cast<float>(yOffset));
  }
  else
  {
    listener->onMouseScroll(static_cast<float>(xOffset), static_cast<float>(yOffset));
  }
}

void Application::framebufferSizeCallback(GLFWwindow *window, int width, int height)
{

  Application *app = static_cast<Application *>(glfwGetWindowUserPointer(window));

  if (!app)
  {
    return;
  }

  int simulationWidth = width - static_cast<int>(app->currentUiWidth);

  glViewport(static_cast<int>(app->currentUiWidth), 0, simulationWidth, height);

  app->screenWidth = width;
  app->screenHeight = height;

  if (app->activeSimulation)
  {
    app->activeSimulation->onResize(simulationWidth, height);
  }
}

void Application::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
  ImGuiIO &io = ImGui::GetIO();
  if (io.WantCaptureKeyboard)
  {
    return;
  }

  Application *app = static_cast<Application *>(glfwGetWindowUserPointer(window));
  if (!app || !app->activeSimulation)
  {
    return;
  }

  if (key == GLFW_KEY_C && action == GLFW_PRESS)
  {
    app->activeSimulation->resetCamera();
  }
}