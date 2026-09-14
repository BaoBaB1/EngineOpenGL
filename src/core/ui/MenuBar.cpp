#include "MenuBar.hpp"
#include "FileExplorer.hpp"
#include "core/Scene.hpp"
#include "core/WindowGLFW.hpp"
#include "core/ModelLoader.hpp"
#include "core/ThreadPool.hpp"
#include "core/AssetManager.hpp"
#include "core/GeometryFactory.hpp"
#include "core/ecs/components/ObjectParametersComponent.hpp"
#include "core/ecs/components/TransformComponent.hpp"
#include "core/ecs/components/DrawableComponent.hpp"
#include "core/ecs/components/DirtyEntityTagComponent.hpp"

#include <imgui.h>
#include <variant>

namespace fury
{
  void MenuBar::init(Scene* scene)
  {
    UiComponent::init(scene);
    m_frame_padding = 7;
    m_height = ImGui::GetFontSize() + 2 * m_frame_padding - 1;
    m_is_visible = true;
  }

  void MenuBar::tick(float)
  {
    if (!is_visible())
    {
      return;
    }
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x, m_frame_padding));
    if (ImGui::BeginMainMenuBar())
    {
      if (ImGui::BeginMenu("File"))
      {
        if (ImGui::MenuItem("Import"))
        {
          auto callback = [this](const std::string& file)
          {
            // Load model in a separate thread
            auto fut = ThreadPool::instance().enqueue(
                [file, this]()
                {
                  ModelLoader l;
                  std::vector<MeshNode> read_nodes = l.load(file);
                  // Only main thread can create OpenGL entities, so at this point texture data
                  // will be read but OpenGL texture must be created by the main thread
                  m_scene->enqueue_job_from_async_thread(
                      [scene = m_scene, nodes = std::move(read_nodes)]()
                      {
                        auto [entity, drawable_component, transform_component] = scene->add_entity(nodes, true);
                        drawable_component.name = "Loaded model";
                        drawable_component.shading_mode = ShadingMode::PREDEFINED;
                      });
                });
          };
          OpenFileExplorerContext ctx;
          ctx.import_asset = true;
          m_scene->get_ui().get_file_explorer().open(ctx, callback);
        }
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Scene"))
      {
        if (ImGui::MenuItem("Save"))
        {
          auto callback = [=, this](const std::string& file)
          {
            ThreadPool::instance().enqueue([scene = m_scene, file]() { scene->save(file); });
          };
          OpenFileExplorerContext ctx;
          ctx.save_scene = true;
          m_scene->get_ui().get_file_explorer().open(ctx, callback);
        }

        if (ImGui::MenuItem("Load"))
        {
          auto callback = [&](const std::string& file)
          {
            m_scene->load(file);
          };
          OpenFileExplorerContext ctx;
          ctx.load_scene = true;
          m_scene->get_ui().get_file_explorer().open(ctx, callback);
        }

        if (ImGui::BeginMenu("Add object"))
        {
          std::string clicked_item_name;
          if (ImGui::MenuItem("Cube"))
          {
            clicked_item_name = "Cube";
          }
          if (ImGui::MenuItem("Pyramid"))
          {
            clicked_item_name = "Pyramid";
          }
          if (ImGui::MenuItem("Icosahedron"))
          {
            clicked_item_name = "Icosahedron";
          }
          ImGui::EndMenu();

          if (!clicked_item_name.empty())
          {
            const ShadingMode default_shading_mode = ShadingMode::FLAT_SHADING;
            AssetHandle<Mesh> mesh;
            std::variant<ecs::CubeParametersComponent, ecs::PyramidParametersComponent,
                         ecs::IcosahedronParametersComponent>
                params_variant;

            if (clicked_item_name == "Cube")
            {
              ecs::CubeParametersComponent params;
              const std::string lookup_name =
                  utils::generate_asset_lookup_name(clicked_item_name, default_shading_mode);
              mesh = MeshManager::get_or_create(lookup_name, [&]()
                                                { return GeometryFactory::create_cube(params, default_shading_mode); });
              params_variant = params;
            }
            else if (clicked_item_name == "Pyramid")
            {
              ecs::PyramidParametersComponent params;
              const std::string lookup_name =
                  utils::generate_asset_lookup_name(clicked_item_name, default_shading_mode);
              mesh = MeshManager::get_or_create(
                  lookup_name, [&]() { return GeometryFactory::create_pyramid(params, default_shading_mode); });
              params_variant = params;
            }
            else if (clicked_item_name == "Icosahedron")
            {
              ecs::IcosahedronParametersComponent params;
              const std::string lookup_name = utils::generate_asset_lookup_name(
                  clicked_item_name, default_shading_mode, params.subdivisiton_depth, params.project_points_on_sphere);
              mesh = MeshManager::get_or_create(
                  lookup_name, [&]() { return GeometryFactory::create_icosahedron(params, default_shading_mode); });
              params_variant = params;
            }
            else
            {
              Logger::error("[MenuBar]. Add object: unknown clicked item.");
              return;
            }

            auto [entity, drawable_component, transform_component] =
                m_scene->add_entity({ MeshNode { mesh } }, true);
            std::visit([scene = m_scene, entity](auto&& params)
                       { scene->get_component_manager().add_component(entity, params); }, params_variant);
            const Ray ray =
                m_scene->get_camera().cast_ray(m_scene->get_window()->width() / 2, m_scene->get_window()->height() / 2);
            constexpr float spawn_distance = 2.5f;
            // spawn in front of camera
            auto spawn_pos = ray.get_origin() + spawn_distance * ray.get_direction();
            drawable_component.shading_mode = default_shading_mode;
            transform_component.transform_node.set_translation(spawn_pos);
            transform_component.transform_node.set_scale(glm::vec3(0.5f));
            utils::set_flag(drawable_component.flags, DrawableFlags::HAS_SURFACE);
          }
        }

        if (ImGui::MenuItem("Create default scene"))
        {
          m_scene->create_default_scene();
        }

        if (ImGui::MenuItem("Clear geometry"))
        {
          m_scene->clear_geometry();
        }

        ImGui::EndMenu();
      }
      ImGui::EndMainMenuBar();
    }
    ImGui::PopStyleVar();
  }
} // namespace fury
