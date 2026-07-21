#include <config.h>

int main()
{
  GLFWwindow *window;

  if (!glfwInit())
  {
    std::cerr << "Failed to initialize GLFW" << std::endl;

    return -1;
  }

  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
  window = glfwCreateWindow(640, 480, "Particle Simulation", NULL, NULL);

  if (window == NULL)
  {
    glfwTerminate();

    std::cerr << "Failed to create GLFW window" << std::endl;

    return -1;
  }

  glfwMakeContextCurrent(window);

  if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
  {
    glfwTerminate();

    std::cerr << "Failed to initialize GLAD" << std::endl;

    return -1;
  }

  glClearColor(0.25f, 0.5f, 0.75f, 1.0f);

  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();

    glClear(GL_COLOR_BUFFER_BIT);

    glfwSwapBuffers(window);
  }

  glfwTerminate();

  return 0;
}