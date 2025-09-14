#include "SceneInfo.hpp"
#include "MenuBar.hpp"
#include "ge/Object3D.hpp"
#include "core/Camera.hpp"
#include "core/SceneRenderer.hpp"
#include "core/WindowGLFW.hpp"
#include "core/ObjectChangeInfo.hpp"
#include "core/TextureManager.hpp"
#include "core/RotationController.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace
{
  using namespace fury;
  const char* shading_mode_to_str(Object3D::ShadingMode mode);
  const char* texture_type_to_str(TextureType type);
  const char* light_type_to_str(LightType type);
}

namespace fury
{
  SceneInfo::SceneInfo(SceneRenderer* scene, MenuBar* menubar) : UiComponent(scene), m_menubar(menubar)
  {
  }

  void SceneInfo::tick()
  {
    if (!is_visible())
      return;

    WindowGLFW* window = m_scene->get_window();

    // stick menu to the right side of window
    const ImVec2 sz = ImVec2(window->width(), window->height());
    constexpr float scale_factor = 0.2f;
    const float menubar_height = m_menubar->get_height();
    ImGui::SetNextWindowSize(ImVec2(sz.x * scale_factor, sz.y - menubar_height));
    ImGui::SetNextWindowPos(ImVec2(sz.x - sz.x * scale_factor, menubar_height));
    ImGui::Begin("Scene properties", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("Scene objects");
    ImGui::PushItemWidth(-1);

    if (ImGui::BeginListBox("##ListBox"))
    {
      int idx = 0;
      for (auto& obj : m_scene->get_drawables())
      {
        bool selected = obj->is_selected();
        if (ImGui::Selectable((obj->get_name() + std::to_string(idx + 1)).c_str(), &selected))
        {
          m_scene->select_object(obj.get(), true);
        }
        ++idx;
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
      if (ImGui::Checkbox("Fill polygons", &m_fill_polygons))
      {
        on_polygon_mode_change.notify(m_fill_polygons ? GL_FILL : GL_LINE);
      }
      if (ImGui::Checkbox("Show scene's bbox", &m_show_scene_bbox))
      {
        on_show_scene_bbox.notify(m_show_scene_bbox);
      }
      if (ImGui::Checkbox("Use MSAA", &m_use_msaa))
      {
        msaa_button_click.notify(m_use_msaa);
      }
      if (ImGui::Checkbox("Show grid", &m_show_grid))
      {
      }
      if (ImGui::Checkbox("VSync", &m_use_vsync))
      {
        if (m_use_vsync)
        {
          m_scene->set_fps_limit(glfwGetVideoMode(glfwGetPrimaryMonitor())->refreshRate);
        }
        else
        {
          m_scene->set_fps_limit(m_fps_cap);
        }
      }
      render_fps_locks();
    }

    // lights section
    const auto lights = m_scene->get_valid_lights();
    static std::vector<const char*> light_names;
    static int selected_light_idx = -1;
    ImGui::Separator();
    ImGui::Text("Lights");
    ImGui::Text(fmt::format("Active lights {}. Valid lights {}.", m_scene->get_active_lights().size(), lights.size()).c_str());
    // clear vector in case some lights were added/removed from scene 
    // (lights.size() check is not reliable, because we could remove and add new light at the same time)
    light_names.clear();
    for (const Light* light : lights)
    {
      light_names.push_back(::light_type_to_str(light->get_type()));
    }
    const int light_list_visible_items = std::min(light_names.size(), 6ULL);
    ImGui::ListBox("##", &selected_light_idx, light_names.data(), light_names.size(), light_list_visible_items);
    if (selected_light_idx != -1)
    {
      const Light* selected_light = lights[selected_light_idx];
      bool active = selected_light->is_enabled();
      if (ImGui::Checkbox("Active", &active))
      {
        if (active)
        {
          const_cast<Light*>(selected_light)->enable();
        }
        else
        {
          const_cast<Light*>(selected_light)->disable();
        }
        light_visibility_toggle.notify(selected_light, active);
      }
      if (selected_light->get_parent())
      {
        ImGui::Text("Attached to %s", selected_light->get_parent()->get_name().c_str());
      }
    }
    
    if (!m_scene->get_selected_objects().empty())
    {
      if (is_general_header_opened)
        ImGui::Dummy({ 0, 5 });
      render_object_properties(*m_scene->get_selected_objects().back());
    }
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Separator();
    ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  }

  void SceneInfo::render_object_properties(Object3D& drawable)
  {
    ImGui::SetNextItemOpen(true, ImGuiCond_::ImGuiCond_Once);
    const std::string& name = drawable.get_name();
    if (ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_CollapsingHeader))
    {
      const ImVec2 wsize = ImGui::GetWindowSize();
      // translation
      ImGui::Separator();
      ImVec2 text_size = ImGui::CalcTextSize("Translation");
      ImGui::SetCursorPosX((wsize.x / 2.f) - (text_size.x / 2.f));
      ImGui::Text("Translation");
      
      ImGui::PushItemWidth(-1);
      // 3 sliders + 2 spacings
      const ImVec2 item_spacing = ImGui::GetStyle().ItemSpacing;
      const float available_width = ImGui::CalcItemWidth() - (item_spacing.x * 2);
      const float slider_width = available_width / 3.f;
      ImGui::PopItemWidth();

      render_xyz_markers(item_spacing.x, slider_width, item_spacing.x);
      m_obj_translation = drawable.translation();
      glm::vec3 old_translation = drawable.translation();
      ObjectChangeInfo transformation_change;
      transformation_change.is_transformation_change = true;

      ImGui::PushItemWidth(slider_width);
      for (int i = 0; i < 3; i++)
      {
        ImGui::PushID(&m_obj_translation.x + i);
        if (ImGui::InputFloat("##translation", &m_obj_translation.x + i))
        {
          reinterpret_cast<glm::vec3&>(drawable.model_matrix()[3]) = m_obj_translation;
          // in world space
          transformation_change.position_change = m_obj_translation - old_translation;
          on_object_change.notify(&drawable, transformation_change);
        }
        ImGui::PopID();
        ImGui::SameLine();
      }
      ImGui::Dummy(ImVec2(0.f, 5.f));
      for (int i = 0; i < 3; i++)
      {
        ImGui::PushID(&m_obj_translation.x + i);
        if (ImGui::SliderFloat("##translation2", &m_obj_translation.x + i, -10.0f, 10.0f))
        {
          reinterpret_cast<glm::vec3&>(drawable.model_matrix()[3]) = m_obj_translation;
          // in world space
          transformation_change.position_change = m_obj_translation - old_translation;
          on_object_change.notify(&drawable, transformation_change);
        }
        ImGui::PopID();
        ImGui::SameLine();
      }

      ImGui::NewLine();
      ImGui::Separator();
      text_size = ImGui::CalcTextSize("Scale");
      ImGui::SetCursorPosX((wsize.x / 2.f) - (text_size.x / 2.f));
      ImGui::Text("Scale");
      render_xyz_markers(item_spacing.x, slider_width, item_spacing.x);
      m_obj_scale = drawable.scale();
      for (int i = 0; i < 3; i++)
      {
        ImGui::PushID(&m_obj_scale.x + i);
        if (ImGui::InputFloat("##scale", &m_obj_scale.x + i))
        {
          if (m_obj_scale.x != 0 && m_obj_scale.y != 0 && m_obj_scale.z != 0)
          {
            drawable.scale(m_obj_scale);
            on_object_change.notify(&drawable, transformation_change);
          }
        }
        ImGui::PopID();
        ImGui::SameLine();
      }
      ImGui::Dummy(ImVec2(0.f, 5.f));
      for (int i = 0; i < 3; i++)
      {
        ImGui::PushID(&m_obj_scale.x + i);
        if (ImGui::SliderFloat("##scale2", &m_obj_scale.x + i, 0.1f, 5.0f))
        {
          drawable.scale(m_obj_scale);
          on_object_change.notify(&drawable, transformation_change);
        }
        ImGui::PopID();
        ImGui::SameLine();
      }
      ImGui::NewLine();
      ImGui::PopItemWidth();

      if (ObjectController* controller = drawable.get_controller(ObjectController::Type::ROTATION))
      {
        ImGui::Separator();
        text_size = ImGui::CalcTextSize("Rotation");
        ImGui::SetCursorPosX((wsize.x / 2.f) - (text_size.x / 2.f));
        ImGui::Text("Rotation");
        bool rotating = controller->is_enabled();
        if (ImGui::Checkbox("Rotate", &rotating))
        {
          if (rotating)
            controller->enable();
          else
            controller->disable();
        }
        const glm::vec3 axis = static_cast<RotationController*>(controller)->get_rotation_axis();
        const float angle = static_cast<RotationController*>(controller)->get_rotation_angle();
        ImGui::TextWrapped("Rotation axis [%.3f, %.3f, %.3f]", axis.x, axis.y, axis.z);
        ImGui::Text("Rotation angle %.3f degrees", angle);
      }

      // other properties
      ImGui::Separator();
      ImGui::PushItemWidth(-1);
      ImGui::Text("Color");
      m_obj_color = drawable.color();
      if (ImGui::ColorEdit4("##Color", &m_obj_color.x))
      {
        drawable.set_color(m_obj_color);
        ObjectChangeInfo info;
        info.is_vertex_change = true;
        on_object_change.notify(&drawable, info);
      }

      ImGui::Separator();
      ImGui::Text("Textures");
      const size_t mesh_count = drawable.mesh_count();
      for (size_t i = 0; i < mesh_count; i++)
      {
        Mesh& mesh = drawable.get_mesh(i);
        if (ImGui::CollapsingHeader(("Mesh" + std::to_string(i)).c_str()))
        {
          for (int j = 0; j < static_cast<int>(TextureType::LAST); j++)
          {
            TextureType tt = static_cast<TextureType>(j);
            // i hate imgui for that!
            // push mesh address, so that it will give unique hash for each object's mesh
            ImGui::PushID(&mesh);
            ImGui::PushID(&j);
            if (ImGui::TreeNode((::texture_type_to_str(tt))))
            {
              int tex_id = 0;
              if (auto tex = mesh.get_texture(tt))
              {
                tex_id = tex->id();
              }
              else
              {
                tex_id = Texture2D::get_placeholder().id();
              }
              if (ImGui::ImageButton("##button", tex_id, ImVec2(50, 50), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f)))
              {
                OpenFileExplorerContext ctx;
                ctx.select_texture = true;
                auto callback = [=, &mesh](const std::string& file)
                  {
                    mesh.set_texture(TextureManager::get(file), tt);
                  };
                m_scene->get_ui().get_component<FileExplorer>("FileExplorer")->open(ctx, callback);
              }
              ImGui::TreePop();
            }
            ImGui::PopID();
            ImGui::PopID();
          }
        }
      }

      ImGui::PopItemWidth();

      ImGui::Separator();
      ImGui::Text("Miscellaneous");
      bool is_bbox_visible = drawable.is_bbox_visible();
      if (ImGui::Checkbox("Show bounding box", &is_bbox_visible))
      {
        drawable.visible_bbox(is_bbox_visible);
        on_visible_bbox_button_pressed.notify(&drawable, is_bbox_visible);
      }

      // TODO: rewrite later as e.g. 2D Circle has surface but it won't be derived from Model class
      if (drawable.has_surface())
      {
        // calc size of next checkbox, if it fits width of properties window, put it on same line,
        // othewise, put on other line
        const auto next_line_cursor_pos = ImGui::GetCursorPos();
        ImGui::SameLine();
        const auto same_line_cursor_pos = ImGui::GetCursorPos();
        const float width_of_next_checkbox = ImGui::CalcTextSize("Visible normals").x
          + ImGui::GetFrameHeight() + ImGui::GetCurrentContext()->Style.ItemInnerSpacing.x;
        constexpr float x_offset = 25;
        if (same_line_cursor_pos.x + x_offset + width_of_next_checkbox > ImGui::GetWindowSize().x)
        {
          ImGui::SetCursorPos(next_line_cursor_pos);
        }
        else {
          ImGui::Dummy(ImVec2(x_offset, 0));
          ImGui::SameLine();
        }

        bool is_normals_visible = drawable.is_normals_visible();
        if (ImGui::Checkbox("Visible normals", &is_normals_visible))
        {
          drawable.visible_normals(is_normals_visible);
          on_visible_normals_button_pressed.notify(&drawable, is_normals_visible);
        }

        if (!drawable.is_fixed_shading())
        {
          // shading modes
          std::vector<std::pair<Object3D::ShadingMode, std::string>> modes(3);
          for (int i = 0; i < 3; i++)
          {
            Object3D::ShadingMode mode = static_cast<Object3D::ShadingMode>(i);
            modes[i] = std::make_pair(mode, ::shading_mode_to_str(mode));
          }
          std::string current_mode = ::shading_mode_to_str(drawable.shading_mode());
          ImGui::SetNextItemWidth(ImGui::GetWindowSize().x / 2);
          if (ImGui::BeginCombo("Shading mode", current_mode.c_str()))
          {
            for (int i = 0; i < 3; i++)
            {
              bool selected = (current_mode == modes[i].second);
              if (ImGui::Selectable(modes[i].second.c_str(), &selected))
              {
                drawable.apply_shading(modes[i].first);
                ObjectChangeInfo info;
                info.is_shading_mode_change = true;
                on_object_change.notify(&drawable, info);
              }
              if (selected)
                ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
          }
        }
      }
    }
  }

  void SceneInfo::render_xyz_markers(float offset_from_left, float item_width, float spacing)
  {
    constexpr static const char* XYZ[] = { "X", "Y", "Z" };
    for (int i = 0; i < 3; i++)
    {
      // center labels over input fields
      ImVec2 size = ImGui::CalcTextSize(XYZ[i]);
      ImGui::SetCursorPosX(offset_from_left + (item_width / 2.f) + (item_width * i) + (spacing * i) - (size.x / 2.f));
      ImGui::Text(XYZ[i]);
      if (i < 2)
        ImGui::SameLine();
    }
  }

  void SceneInfo::render_fps_locks()
  {
    static constexpr std::array<std::pair<std::string_view, int>, 8> fps_locks = { {
        {"0", 0}, {"30", 30}, {"60", 60}, {"120", 120}, {"144", 144}, {"180", 180}, {"240", 240}, {"360", 360}} };
    static const char* selected_fps_lock = nullptr;
    static bool once = true;
    if (once)
    {
      const uint32_t fps_lock = m_scene->get_fps_limit();
      for (const auto& [label, fps] : fps_locks)
      {
        if (fps_lock == fps)
        {
          selected_fps_lock = label.data();
          m_fps_cap = fps;
          break;
        }
      }
      once = false;
    }
    if (m_use_vsync)
      ImGui::BeginDisabled();
    if (ImGui::BeginCombo("FPS lock", selected_fps_lock))
    {
      for (const auto& [label, fps] : fps_locks)
      {
        bool is_selected = (selected_fps_lock && (selected_fps_lock == label));
        if (ImGui::Selectable(label.data(), is_selected))
        {
          selected_fps_lock = label.data();
          m_scene->set_fps_limit(fps);
          m_fps_cap = fps;
        }
        if (is_selected)
        {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
    if (m_use_vsync)
      ImGui::EndDisabled();
  }
}

namespace
{
  // TODO: magic_enum
  using namespace fury;
  const char* shading_mode_to_str(Object3D::ShadingMode mode)
  {
    switch (mode)
    {
    case Object3D::ShadingMode::SMOOTH_SHADING:
      return "Smooth shading";
    case Object3D::ShadingMode::FLAT_SHADING:
      return "Flat shading";
    case Object3D::ShadingMode::NO_SHADING:
      return "No shading";
    default:
      return "Unknown";
    }
  }

  const char* texture_type_to_str(TextureType type)
  {
    switch (type)
    {
    case TextureType::GENERIC:
      return "Generic texture";
    case TextureType::DIFFUSE:
      return "Diffuse texture";
    case TextureType::AMBIENT:
      return "Ambient texture";
    case TextureType::SPECULAR:
      return "Specular texture";
    default:
      return "Unknown texture";
    }
  }

  const char* light_type_to_str(LightType type)
  {
    switch (type)
    {
    case LightType::DIRECTIONAL:
      return "Directional light";
    case LightType::POINT:
      return "Point light";
    case LightType::SPOT:
      return "Spot light";
    default:
      return "Unknown light";
    }
  };
}
