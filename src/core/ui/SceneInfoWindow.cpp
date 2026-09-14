#include "SceneInfoWindow.hpp"
#include "MenuBar.hpp"
#include "core/Camera.hpp"
#include "core/Scene.hpp"
#include "core/WindowGLFW.hpp"
#include "core/ShadingProcessor.hpp"
#include "core/GeometryFactory.hpp"
#include "core/AssetManager.hpp"
#include "core/ecs/components/LightComponent.hpp"
#include "core/ecs/components/DrawableComponent.hpp"
#include "core/ecs/components/TransformComponent.hpp"
#include "core/ecs/components/RotationComponent.hpp"
#include "core/ecs/components/DirtyEntityTagComponent.hpp"
#include "FileExplorer.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace fury
{
  static constexpr const char* light_type_to_str(ecs::LightType type)
  {
    switch (type)
    {
    case ecs::LightType::DIRECTIONAL:
      return "Directional light";
    case ecs::LightType::POINT:
      return "Point light";
    case ecs::LightType::SPOT:
      return "Spot light";
    default:
      return "Unknown light type";
    }
  };

  static constexpr const char* shading_mode_to_str(ShadingMode mode)
  {
    switch (mode)
    {
    case ShadingMode::SMOOTH_SHADING:
      return "Smooth shading";
    case ShadingMode::FLAT_SHADING:
      return "Flat shading";
    case ShadingMode::NO_SHADING:
      return "No shading";
    case ShadingMode::PREDEFINED:
      return "Predefined";
    default:
      return "Unknown";
    }
  }

  void SceneInfoWindow::init(Scene* scene)
  {
    UiComponent::init(scene);
    scene->get_window()->window_size_changed += new InstanceListener(this, &SceneInfoWindow::calculate_window_size);
    calculate_window_size(scene->get_window()->width(), scene->get_window()->height());
  }

  void SceneInfoWindow::calculate_window_size(int w, int h)
  {
    const float menubar_height = m_scene->get_ui().get_menubar().get_height();
    constexpr float scale_factor = 0.2f;
    m_window_size.x = w * scale_factor;
    m_window_size.y = h - menubar_height;
    m_window_pos.x = w - w * scale_factor;
    m_window_pos.y = menubar_height * 2;
  }

  void SceneInfoWindow::tick(float)
  {
    if (!is_visible())
    {
      return;
    }

    ImGui::SetNextWindowSize(ImVec2(m_window_size.x, m_window_size.y));
    ImGui::SetNextWindowPos(ImVec2(m_window_pos.x, m_window_pos.y));

    ImGui::Begin("Scene properties", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("Scene objects");
    ImGui::PushItemWidth(-1);

    if (ImGui::BeginListBox("##ListBox"))
    {
      const auto& component_manager = m_scene->get_component_manager();
      const EntityId selected_entity = m_scene->get_selected_entity();
      for (auto [entity, component] : component_manager.get_components_zipped<ecs::DrawableComponent>())
      {
        bool selected = (entity == selected_entity);
        ImGui::PushID(&component);
        if (ImGui::Selectable(component.name.c_str(), &selected))
        {
          m_scene->select_object(entity, true);
        }
        ImGui::PopID();
      }
      ImGui::EndListBox();
    }
    ImGui::PopItemWidth();
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 5));

    ImGui::SetNextItemOpen(true, ImGuiCond_::ImGuiCond_Once);
    bool is_general_header_opened = false;
    if (ImGui::CollapsingHeader("General"))
    {
      is_general_header_opened = true;
      SceneOptions& options = m_scene->get_options();
      // TODO: check if if is needed
      if (ImGui::Checkbox("Fill polygons", &options.fill_polygons))
      {
        fill_polygons_button_toggled.notify(options.fill_polygons);
      }
      if (ImGui::Checkbox("Show scene's bbox", &options.show_scene_bbox))
      {
        show_scene_bbox_button_toggled.notify(options.show_scene_bbox);
      }
      if (ImGui::Checkbox("Use MSAA", &options.use_msaa))
      {
        use_msaa_button_toggled.notify(options.use_msaa);
      }
      if (ImGui::Checkbox("Show grid", &options.show_grid))
      {
        show_grid_button_toggled.notify(options.show_grid);
      }
      if (ImGui::Checkbox("Frustum culling", &options.is_frustum_culling_enabled))
      {
        frustum_culling_button_toggled.notify(options.is_frustum_culling_enabled);
      }
      if (ImGui::Checkbox("VSync", &options.is_vsync_enabled))
      {
        if (options.is_vsync_enabled)
        {
          options.fps_cap = glfwGetVideoMode(glfwGetPrimaryMonitor())->refreshRate;
        }
        m_scene->set_fps_limit(options.fps_cap);
      }
      render_fps_locks();
    }

    auto light_components = m_scene->get_component_manager().get_components_zipped<ecs::LightComponent>();

    // Lights section
    static std::vector<const char*> light_names;
    ImGui::Separator();
    ImGui::Text("Lights");
    // clear vector in case some lights were added/removed from scene
    // (lights.size() check is not reliable, because we could remove and add new light at the same time)
    light_names.clear();
    for (auto [entity, light] : light_components)
    {
      light_names.push_back(light_type_to_str(light.type));
    }
    const int light_list_visible_items = std::min<int>(light_names.size(), 6);
    static int selected_light_idx = -1;
    if (ImGui::ListBox("##", &selected_light_idx, light_names.data(), light_names.size(), light_list_visible_items))
    {
    }
    if (selected_light_idx != -1)
    {
      ecs::LightComponent& selected_light_component = light_components[selected_light_idx].second;
      bool is_active = selected_light_component.is_active;
      if (ImGui::Checkbox("Active", &is_active))
      {
        const EntityId light_entity = light_components[selected_light_idx].first;
        ecs::DirtyEntityTagComponent& dirty_tag =
            m_scene->get_component_manager().get_or_create_component<ecs::DirtyEntityTagComponent>(light_entity);
        selected_light_component.is_active = is_active;
        //dirty_tag.dirty_bits.active_state_changed = true;
      }
      /* if (transform->has_parent())
      {
        transform.transform_node.get_parent()->get_children
        ImGui::Text("Attached to %s", transform->get_parent()->get_owner()->get_name().c_str());
      }*/
    }

    const EntityId selected_entity = m_scene->get_selected_entity();
    if (selected_entity != g_invalid_entity_id)
    {
      if (is_general_header_opened)
      {
        ImGui::Dummy({ 0, 5 });
      }
      render_entity_properties(selected_entity);
    }

    if (m_scene->get_options().is_frustum_culling_enabled)
    {
      ImGui::Separator();
      ImGui::Text(fmt::format("{} objects culled", m_scene->get_frame_info().num_objects_culled).c_str());
    }

    ImGui::Separator();
    const FrameInfo& info = m_scene->get_frame_info();
    ImGui::Text("Frame time %.3f ms", info.frame_time * 1000.f);
    ImGui::Text("FPS %d", m_scene->get_fps());
    ImGui::End();
  }

  void SceneInfoWindow::render_entity_properties(EntityId entity)
  {
    ImGui::SetNextItemOpen(true, ImGuiCond_::ImGuiCond_Once);

    auto& drawable_component = m_scene->get_component_manager().get_component<ecs::DrawableComponent>(entity);
    auto& transform_component = m_scene->get_component_manager().get_component<ecs::TransformComponent>(entity);

    if (ImGui::TreeNodeEx(drawable_component.name.c_str(), ImGuiTreeNodeFlags_CollapsingHeader))
    {
      const ImVec2 wsize = ImGui::GetWindowSize();
      // translation
      ImGui::Separator();

      const TransformationSceneNode& transform_node = transform_component.transform_node;
      const glm::vec3& translation = transform_node.get_translation();
      const glm::vec3& scale = transform_node.get_scale();
      ImGui::TextWrapped("Translation: x = %.4f, y = %.4f, z = %.4f", translation.x, translation.y, translation.z);
      ImGui::TextWrapped("Scale: x = %.4f, y = %.4f, z = %.4f", scale.x, scale.y, scale.z);
      ImGui::NewLine();
      if (auto rot_comp = m_scene->get_component_manager().get_component_safe<ecs::RotationComponent>(entity))
      {
        ImGui::Separator();
        const ImVec2 text_size = ImGui::CalcTextSize("Rotation");
        ImGui::SetCursorPosX((wsize.x / 2.f) - (text_size.x / 2.f));
        ImGui::Text("Rotation");
        // bool rotating = controller->is_enabled();
        // if (ImGui::Checkbox("Rotate", &rotating))
        //{
        //   if (rotating)
        //   {
        //     controller->enable();
        //   }
        //   else
        //   {
        //     controller->disable();
        //   }
        // }
        //  const glm::vec3 axis = static_cast<RotationController*>(controller.get())->get_rotation_axis();
        //  const float angle = static_cast<RotationController*>(controller.get())->get_rotation_angle();
        ImGui::TextWrapped("Rotation axis [%.3f, %.3f, %.3f]", rot_comp->axis.x, rot_comp->axis.y, rot_comp->axis.z);
        ImGui::Text("Rotation angle %.3f degrees", rot_comp->angle);
      }

      // other properties
      ImGui::Separator();
      ImGui::PushItemWidth(-1);
      ImGui::Text("Color");
      if (ImGui::ColorEdit4("##Color", &drawable_component.color.x))
      {
        ecs::DirtyEntityTagComponent dirty_tag =
            m_scene->get_component_manager().get_or_create_component<ecs::DirtyEntityTagComponent>(entity);
        dirty_tag.dirty_bits.color_changed = true;
      }

      ImGui::Separator();
      ImGui::Text("Textures");

      for (size_t i = 0; auto& mesh_node : drawable_component.mesh_nodes)
      {
        if (ImGui::CollapsingHeader(("Mesh" + std::to_string(i++)).c_str()))
        {
          constexpr static const char* texture_names[] = { "Ambient", "Diffuse", "Specular" };
          std::array<AssetHandle<Texture2D>, 3> textures;
          textures.fill(Texture2D::get_placeholder());

          if (mesh_node.material)
          {
            const Material& mesh_material = *mesh_node.material.asset;
            if (mesh_material.ambient_tex)
            {
              textures[0] = mesh_material.ambient_tex;
            }
            if (mesh_material.diffuse_tex)
            {
              textures[1] = mesh_material.diffuse_tex;
            }
            if (mesh_material.specular_tex)
            {
              textures[2] = mesh_material.specular_tex;
            }
          }
          for (int j = 0; j < textures.size(); j++)
          {
            ImGui::PushID(j);
            if (ImGui::TreeNode(texture_names[j]))
            {
              if (ImGui::ImageButton("##button", textures[j].asset->id(), ImVec2(50, 50), ImVec2(0.0f, 1.0f),
                                     ImVec2(1.0f, 0.0f)))
              {
                OpenFileExplorerContext ctx;
                ctx.select_texture = true;
                auto callback = [texture_name = std::string(texture_names[j]), &mesh_node,
                                 &drawable_component](const std::string& file)
                {
                  if (!mesh_node.material || mesh_node.material.asset.use_count() > 1)
                  {
                    mesh_node.material.asset = std::make_shared<Material>();
                  }
                  if (texture_name == "Ambient")
                  {
                    mesh_node.material.asset->ambient_tex = TextureManager::load(file);
                  }
                  else if (texture_name == "Diffuse")
                  {
                    mesh_node.material.asset->diffuse_tex = TextureManager::load(file);
                  }
                  else if (texture_name == "Specular")
                  {
                    mesh_node.material.asset->specular_tex = TextureManager::load(file);
                  }
                };
                m_scene->get_ui().get_file_explorer().open(ctx, callback);
              }
              ImGui::TreePop();
            }
            ImGui::PopID();
          }
        }
      }
    }

    ImGui::Separator();
    ImGui::Text("Miscellaneous");
    bool is_bbox_visible = m_scene->get_entities_with_visible_bboxes().contains(entity);
    if (ImGui::Checkbox("Show bounding box", &is_bbox_visible))
    {
      visible_bbox_button_toggled.notify(entity, is_bbox_visible);
    }

    // TODO: rewrite later as e.g. 2D Circle has surface but it won't be derived from Model class
    if (utils::get_flag(drawable_component.flags, DrawableFlags::HAS_SURFACE))
    {
      // calc size of next checkbox, if it fits width of properties window, put it on same line,
      // othewise, put on other line
      const auto next_line_cursor_pos = ImGui::GetCursorPos();
      ImGui::SameLine();
      const auto same_line_cursor_pos = ImGui::GetCursorPos();
      const float width_of_next_checkbox = ImGui::CalcTextSize("Visible normals").x + ImGui::GetFrameHeight() +
                                           ImGui::GetCurrentContext()->Style.ItemInnerSpacing.x;
      constexpr float x_offset = 25;
      if (same_line_cursor_pos.x + x_offset + width_of_next_checkbox > ImGui::GetWindowSize().x)
      {
        ImGui::SetCursorPos(next_line_cursor_pos);
      }
      else
      {
        ImGui::Dummy(ImVec2(x_offset, 0));
        ImGui::SameLine();
      }

      bool is_normals_visible = m_scene->get_entities_with_visible_normals().contains(entity);
      if (ImGui::Checkbox("Visible normals", &is_normals_visible))
      {
        visible_normals_button_toggled.notify(entity, is_normals_visible);
      }

      constexpr static std::pair<std::string_view, ShadingMode> shading_modes[] = {
        { shading_mode_to_str(ShadingMode::NO_SHADING), ShadingMode::NO_SHADING },
        { shading_mode_to_str(ShadingMode::FLAT_SHADING), ShadingMode::FLAT_SHADING },
        { shading_mode_to_str(ShadingMode::SMOOTH_SHADING), ShadingMode::SMOOTH_SHADING },
        //{ ::shading_mode_to_str(ShadingMode::PREDEFINED), ShadingMode::PREDEFINED }
      };
      const std::string_view current_shading_mode = shading_mode_to_str(drawable_component.shading_mode);
      ImGui::SetNextItemWidth(ImGui::GetWindowSize().x / 2);
      // If shading is predefined, then we cannot change shading mode (e.g. loaded model)
      ImGui::BeginDisabled(drawable_component.shading_mode == ShadingMode::PREDEFINED);
      if (ImGui::BeginCombo("Shading mode", current_shading_mode.data()))
      {
        for (const auto [mode_str, mode] : shading_modes)
        {
          bool selected = (current_shading_mode == mode_str);
          if (ImGui::Selectable(mode_str.data(), &selected))
          {
            drawable_component.shading_mode = mode;
            // std::vector<MeshNode> new_mesh_nodes;
            // new_mesh_nodes.reserve(drawable_component.mesh_nodes.size());
            //  Cube or pyramid or icosahedron for now
            AssetHandle<Mesh> new_mesh;
            if (auto c = m_scene->get_component_manager().get_component_safe<ecs::CubeParametersComponent>(entity))
            {
              new_mesh = MeshManager::get_or_create(utils::generate_asset_lookup_name("Cube", mode),
                                                    [&]() { return GeometryFactory::create_cube(*c, mode); });
            }
            else if (auto c = m_scene->get_component_manager().get_component_safe<ecs::IcosahedronParametersComponent>(
                         entity))
            {
              new_mesh = MeshManager::get_or_create(utils::generate_asset_lookup_name("Icosahedron", mode,
                                                                                      c->subdivisiton_depth,
                                                                                      c->project_points_on_sphere),
                                                    [&]() { return GeometryFactory::create_icosahedron(*c, mode); });
            }
            else if (auto c =
                         m_scene->get_component_manager().get_component_safe<ecs::PyramidParametersComponent>(entity))
            {
              new_mesh = MeshManager::get_or_create(utils::generate_asset_lookup_name("Pyramid", mode),
                                                    [&]() { return GeometryFactory::create_pyramid(*c, mode); });
            }
            else
            {
              Logger::warn("Failed to change shading mode for entity. Entity type is unknown.");
            }

            if (new_mesh)
            {
              assert(drawable_component.mesh_nodes.size() == 1);
              drawable_component.mesh_nodes[0].mesh = new_mesh;
              ecs::DirtyEntityTagComponent& dirty_tag =
                  m_scene->get_component_manager().get_or_create_component<ecs::DirtyEntityTagComponent>(entity);
              dirty_tag.dirty_bits.shading_mode_changed = 1;
            }
          }
          if (selected)
          {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
      ImGui::EndDisabled();
    }
  }

  void SceneInfoWindow::render_fps_locks()
  {
    static constexpr std::array<std::pair<std::string_view, int>, 8> fps_locks = { { { "0", 0 },
                                                                                     { "30", 30 },
                                                                                     { "60", 60 },
                                                                                     { "120", 120 },
                                                                                     { "144", 144 },
                                                                                     { "180", 180 },
                                                                                     { "240", 240 },
                                                                                     { "360", 360 } } };
    static const char* selected_fps_lock = nullptr;
    static bool once = true;
    SceneOptions& scene_options = m_scene->get_options();
    if (once)
    {
      const uint32_t fps_lock = m_scene->get_fps_limit();
      for (const auto& [label, fps] : fps_locks)
      {
        if (fps_lock == fps)
        {
          selected_fps_lock = label.data();
          scene_options.fps_cap = fps;
          // m_fps_cap = fps;
          break;
        }
      }
      once = false;
    }
    if (scene_options.is_vsync_enabled)
    {
      ImGui::BeginDisabled();
    }
    if (ImGui::BeginCombo("FPS lock", selected_fps_lock))
    {
      for (const auto& [label, fps] : fps_locks)
      {
        bool is_selected = (selected_fps_lock && (selected_fps_lock == label));
        if (ImGui::Selectable(label.data(), is_selected))
        {
          selected_fps_lock = label.data();
          m_scene->set_fps_limit(fps);
          scene_options.fps_cap = fps;
        }
        if (is_selected)
        {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
    if (scene_options.is_vsync_enabled)
    {
      ImGui::EndDisabled();
    }
  }
} // namespace fury
