#include "MenuBar.hpp"
#include "core/Scene.hpp"
#include "core/WindowGLFW.hpp"
#include "ge/Pyramid.hpp"
#include "ge/Icosahedron.hpp"
#include "ge/Cube.hpp"
#include "core/ModelLoader.hpp"
#include "core/SceneGraphManager.hpp"
#include "core/ThreadPool.hpp"
#include "FileExplorer.hpp"

#include <imgui.h>

namespace fury
{
  MenuBar::MenuBar(Scene* scene) : UiComponent(scene)
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
          auto callback = [scene = this->m_scene](const std::string& file)
          {
            // Load model in a separate thread
            auto fut = ThreadPool::instance().enqueue(
                [file]() -> std::unique_ptr<Object3D>
                {
                  ModelLoader loader;
                  const unsigned int assimp_flags = aiProcess_Triangulate | aiProcess_FlipUVs |
                                                    aiProcess_JoinIdenticalVertices |
                                                    aiProcess_GenBoundingBoxes /*| aiProcess_GenSmoothNormals*/;
                  return loader.load(file, assimp_flags);
                });
            scene->add_async_task(std::move(fut), DeferredCommandContext::ADD_OBJECT);
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
          std::unique_ptr<Object3D> obj;
          if (ImGui::MenuItem("Cube"))
          {
            obj = std::make_unique<Cube>();
            clicked = true;
          }
          if (ImGui::MenuItem("Pyramid"))
          {
            obj = std::make_unique<Pyramid>();
            clicked = true;
          }
          if (ImGui::MenuItem("Icosahedron"))
          {
            obj = std::make_unique<Icosahedron>();
            clicked = true;
          }
          ImGui::EndMenu();

          if (clicked)
          {
            TransformationSceneNode* transform =
                SceneGraphManager::get_entity_node<TransformationSceneNode>(obj->get_id());
            auto ray =
                m_scene->get_camera().cast_ray(m_scene->get_window()->width() / 2, m_scene->get_window()->height() / 2);
            static constexpr float spawn_distance = 2.5f;
            // spawn in front of camera
            auto spawn_pos = ray.get_origin() + spawn_distance * ray.get_direction();
            transform->set_translation(spawn_pos);
            transform->set_scale(glm::vec3(0.5f));
            m_scene->add_object(std::move(obj));
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
} // namespace fury
