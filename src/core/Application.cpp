#include "Application.hpp"
#include "Scene.hpp"
#include "Logger.hpp"
#include "AssetManager.hpp"
#include "ShaderStorage.hpp"
#include <GLFW/glfw3.h>
#include <stdexcept>

static void setup_opengl()
{
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glEnable(GL_STENCIL_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);
  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_MULTISAMPLE);
}

namespace fury
{
  Application::Application()
  {
    if (glfwInit() != GLFW_TRUE)
    {
      Logger::critical("Failed to init glfw");
      throw std::runtime_error("Failed to init glfw");
    }
    m_window.init(1600, 900, "MainWindow");
    AssetManager::init();
    ShaderStorage::init();
    InputSystem::instance().init(&m_window);
    Scene::instance().init(&m_window);
    ::setup_opengl();
  }

  Application::~Application()
  {
    glfwTerminate();
  }

  void Application::run()
  {
    Scene::instance().render();
  }
}
