#include "Ui.hpp"
#include "UiComponent.hpp"
#include "core/Scene.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

namespace fury
{
  void Ui::init(Scene* scene)
  {
    m_components = { &m_menubar, &m_file_explorer, &m_gizmo, &m_scene_info_window, &m_item_selection_wheel };
    for (UiComponent* component : m_components)
    {
      component->init(scene);
    }
  }

  void Ui::tick(float dt)
  {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    for (const auto& component : m_components)
    {
      component->tick(dt);
    }
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  }
} // namespace fury
