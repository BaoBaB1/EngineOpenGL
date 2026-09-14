#include "Application.hpp"
#include "Scene.hpp"
#include "Logger.hpp"
#include "ShaderStorage.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <thread>

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

std::thread::id g_main_thread_id;

namespace fury
{
  static void init_imgui(GLFWwindow* window)
  {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    ImGui_ImplGlfw_InitForOpenGL(
        window, true); // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init();
  }

  static void deinit_imgui()
  {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
  }

  Application::Application()
  {
    ::g_main_thread_id = std::this_thread::get_id();
    InputSystem::instance().init(&m_window);
    // Must init after InputSystem::init or it'll overwrite imgui's callbacks and make widgets not responsible
    init_imgui(m_window.gl_window());
    ShaderStorage::init();
    m_scene.init(&m_window);
    ::setup_opengl();
  }

  Application::~Application()
  {
    deinit_imgui();
    glfwTerminate();
  }

  void Application::run()
  {
    m_scene.render();
  }
} // namespace fury
