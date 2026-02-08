#include "MenuBar.hpp"
#include "core/SceneRenderer.hpp"
#include "core/WindowGLFW.hpp"
#include "ge/Pyramid.hpp"
#include "ge/Icosahedron.hpp"
#include "ge/Cube.hpp"
#include "core/ModelLoader.hpp"
#include "core/SceneGraphManager.hpp"

#include <imgui.h>

namespace fury
{
  MenuBar::MenuBar(SceneRenderer* scene) : UiComponent(scene)
  {
    m_is_visible = true;
  }

  void MenuBar::tick(float)
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
      m_height = ImGui::GetWindowHeight();
      if (ImGui::BeginMenu("File"))
      {
        if (ImGui::MenuItem("Import"))
        {
          auto callback = [&](const std::string& file)
            {
              auto& p_new_obj = m_scene->get_drawables().emplace_back();
              const unsigned int assimp_flags = aiProcess_Triangulate | aiProcess_FlipUVs
                | aiProcess_JoinIdenticalVertices | aiProcess_GenBoundingBoxes /*| aiProcess_GenSmoothNormals*/;
              ModelLoader loader;
              if (loader.load(file, assimp_flags, *p_new_obj))
              {
                m_scene->on_new_object_added.notify(p_new_obj.get());
              }
              else
              {
                m_scene->get_drawables().pop_back();
              }
            };
          OpenFileExplorerContext ctx;
          ctx.import_asset = true;
          m_scene->get_ui().get_component<FileExplorer>("FileExplorer")->open(ctx, callback);
        }
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Scene"))
      {
        if (ImGui::MenuItem("Save"))
        {
          auto callback = [&](const std::string& file)
            {
              m_scene->save(file);
            };
          OpenFileExplorerContext ctx;
          ctx.save_scene = true;
          m_scene->get_ui().get_component<FileExplorer>("FileExplorer")->open(ctx, callback);
        }

        if (ImGui::MenuItem("Load"))
        {
          auto callback = [&](const std::string& file)
            {
              m_scene->load(file);
            };
          OpenFileExplorerContext ctx;
          ctx.load_scene = true;
          m_scene->get_ui().get_component<FileExplorer>("FileExplorer")->open(ctx, callback);
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
            auto node = SceneGraphManager::get_entity_node<TransformationSceneNode>(added_obj->get_id());
            node->set_translation(spawn_pos);
            node->set_scale(glm::vec3(0.5f));
            m_scene->on_new_object_added.notify(added_obj);
          }
        }

        if (ImGui::MenuItem("Create default scene"))
        {
          m_scene->create_default_scene();
        }

        if (ImGui::MenuItem("Clear"))
        {
          m_scene->clear();
        }

        ImGui::EndMenu();

      }
      ImGui::EndMainMenuBar();
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
  }
}
