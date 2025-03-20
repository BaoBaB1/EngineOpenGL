#include "SceneRenderer.hpp"
#include "WindowGLFW.hpp"

int main() 
{
  glfwInit();
  WindowGLFW window(1600, 900, "MainWindow");
  auto& scene = SceneRenderer(&window);
  scene.render();
  glfwTerminate();
  return 0;
}
