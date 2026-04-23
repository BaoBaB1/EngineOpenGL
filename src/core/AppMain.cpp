#include "SceneRenderer.hpp"
#include "WindowGLFW.hpp"
#include "Logger.hpp"
#include <GLFW/glfw3.h>

int main()
{
  using namespace fury;
  if (glfwInit() != GLFW_TRUE)
  {
    Logger::critical("Failed to init glfw");
    return -1;
  }
  WindowGLFW window(1600, 900, "MainWindow");
  SceneRenderer::instance().init(&window);
  SceneRenderer::instance().render();
  glfwTerminate();
  return 0;
}
