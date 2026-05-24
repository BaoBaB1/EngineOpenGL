#include "Ui.hpp"
#include "UiComponent.hpp"
#include "SceneInfo.hpp"
#include "MenuBar.hpp"
#include "FileExplorer.hpp"
#include "Gizmo.hpp"
#include "core/Scene.hpp"
#include "core/WindowGLFW.hpp"

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

namespace fury
{
  Ui::Ui()
  {
  }

  Ui::Ui(Scene* scene)
  {
    init(scene);
  }

  void Ui::init(Scene* scene)
  {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    ImGui_ImplGlfw_InitForOpenGL(
        scene->get_window()->gl_window(),
        true); // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init();

    m_components["MenuBar"] = std::make_unique<MenuBar>(scene);
    m_components["SceneInfo"] =
        std::make_unique<SceneInfo>(scene, static_cast<MenuBar*>(m_components.at("MenuBar").get()));
    m_components["Gizmo"] = std::make_unique<Gizmo>(scene);
    m_components["FileExplorer"] = std::make_unique<FileExplorer>(scene);
  }

  Ui::~Ui()
  {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
  }

  template<typename T>
  T* Ui::get_component(const std::string& name)
  {
    return static_cast<T*>(m_components.at(name).get());
  }

  UiComponent* Ui::get_component(const std::string& name)
  {
    return m_components.at(name).get();
  }

  void Ui::tick(float dt)
  {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    for (const auto& component : m_components)
    {
      component.second->tick(dt);
    }
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  }

  template SceneInfo* Ui::get_component<SceneInfo>(const std::string& name);
  template FileExplorer* Ui::get_component<FileExplorer>(const std::string& name);
  template Gizmo* Ui::get_component<Gizmo>(const std::string& name);
  template MenuBar* Ui::get_component<MenuBar>(const std::string& name);
} // namespace fury
