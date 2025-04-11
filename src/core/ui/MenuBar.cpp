#include "MenuBar.hpp"
#include "core/SceneRenderer.hpp"
#include "core/WindowGLFW.hpp"
#include "ge/Pyramid.hpp"
#include "ge/Icosahedron.hpp"
#include "ge/Cube.hpp"

namespace fury
{
  MenuBar::MenuBar(SceneRenderer* scene) : UiComponent(scene)
  {
    m_is_visible = true;
  }

  void MenuBar::tick()
  {
    if (!is_visible())
    {
      return;
    }
    static constexpr ImVec2 menubar_frame_padding = ImVec2(0, 10);
    static constexpr ImVec2 menubar_item_spacing = ImVec2(29, 10);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, menubar_frame_padding);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, menubar_item_spacing);
    if (ImGui::BeginMainMenuBar())
    {
      if (ImGui::BeginMenu("File"))
      {
        if (ImGui::MenuItem("Import"))
        {
          on_open_file_explorer.notify(OpenFileExplorerContext(false, false));
        }
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Scene"))
      {
        if (ImGui::MenuItem("Save"))
        {
          on_open_file_explorer.notify(OpenFileExplorerContext(true, true));
        }

        if (ImGui::MenuItem("Load"))
        {
          on_open_file_explorer.notify(OpenFileExplorerContext(false, true));
        }
        
        if (ImGui::BeginMenu("Add object"))
        {
          bool clicked = false;
          if (ImGui::MenuItem("Cube"))
          {
            m_scene->get_drawables().emplace_back(std::make_unique<Cube>());
            clicked = true;
          }
          if (ImGui::MenuItem("Pyramid"))
          {
            m_scene->get_drawables().emplace_back(std::make_unique<Pyramid>());
            clicked = true;
          }
          if (ImGui::MenuItem("Icosahedron"))
          {
            m_scene->get_drawables().emplace_back(std::make_unique<Icosahedron>());
            clicked = true;
          }
          ImGui::EndMenu();

          if (clicked)
          {
            auto ray = m_scene->get_camera().cast_ray(m_scene->get_window()->width() / 2, m_scene->get_window()->height() / 2);
            static constexpr float spawn_distance = 2.5f;
            // spawn in front of camera
            auto spawn_pos = ray.get_origin() + spawn_distance * ray.get_direction();
            Object3D* added_obj = m_scene->get_drawables().back().get();
            added_obj->translate(spawn_pos);
            added_obj->scale(glm::vec3(0.5f));
            m_scene->on_new_object_added.notify(added_obj);
          }
        }

        ImGui::EndMenu();

      }
      ImGui::EndMainMenuBar();
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
  }
}
