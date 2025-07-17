#include "Ui.hpp"
#include "core/SceneRenderer.hpp"
#include "core/WindowGLFW.hpp"

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

namespace fury
{
  Ui::Ui(SceneRenderer* scene)
  {
    init(scene);
  }

  void Ui::init(SceneRenderer* scene)
  {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    ImGui_ImplGlfw_InitForOpenGL(scene->get_window()->gl_window(), true); // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init();

    m_components["MenuBar"] = std::make_unique<MenuBar>(scene);
    m_components["SceneInfo"] = std::make_unique<SceneInfo>(scene, static_cast<MenuBar*>(m_components.at("MenuBar").get()));
    m_components["Gizmo"] = std::make_unique<Gizmo>(scene);
    m_components["FileExplorer"] = std::make_unique<FileExplorer>(scene);
  }

  Ui::~Ui()
  {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
  }

  void Ui::tick()
  {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    for (const auto& component : m_components)
    {
      component.second->tick();
    }
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  }
}
