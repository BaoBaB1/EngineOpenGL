#include "SceneRenderer.hpp"
#include "WindowGLFW.hpp"
#include <GLFW/glfw3.h>

int main()
{
  glfwInit();
  fury::WindowGLFW window(1600, 900, "MainWindow");
  fury::SceneRenderer::instance().init(&window);
  fury::SceneRenderer::instance().render();
  glfwTerminate();
  return 0;
}
