#include <config.h>
#include "application.h"
#include "simulations/clothSimulation.h"
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

  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Particle Simulation", NULL, NULL);

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
  ImGui_ImplOpenGL3_Init("#version 400");
  glClearColor(0.5f, 0.5f, 0.5f, 1.0f);

  activeSimulation = new ClothSimulation(40, 30, 15.0f, 100.0f, 50.0f, SCREEN_WIDTH, SCREEN_HEIGHT);
  activeSimulation->init();

  syncSimulationResolution();

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

    ImGui::Begin("Simulation Controls");

    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Frame time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);

    ImGui::Separator();

    if (activeSimulation)
    {
      activeSimulation->renderUI();
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

    glClear(GL_COLOR_BUFFER_BIT);

    if (activeSimulation)
    {
      activeSimulation->render();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
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

  listener->onMouseMove(static_cast<float>(xPosition), static_cast<float>(yPosition));
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

  bool isCtrlDown = (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
                     glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS);

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
  glViewport(0, 0, width, height);

  Application *app = static_cast<Application *>(glfwGetWindowUserPointer(window));
  if (!app || !app->activeSimulation)
  {
    return;
  }

  app->syncSimulationResolution();
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

void Application::syncSimulationResolution()
{
  if (!activeSimulation)
  {
    return;
  }

  int fbWidth, fbHeight;
  glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

  activeSimulation->onResize(fbWidth, fbHeight);
}