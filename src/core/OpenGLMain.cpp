#include "SceneRenderer.hpp"
#include "WindowGLFW.hpp"
#include <GLFW/glfw3.h>

int main()
{
  glfwInit();
  fury::WindowGLFW window(1600, 900, "MainWindow");
  auto scene = fury::SceneRenderer(&window);
  scene.render();
  glfwTerminate();
  return 0;
}
