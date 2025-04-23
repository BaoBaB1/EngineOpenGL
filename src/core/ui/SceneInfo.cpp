#include "SceneInfo.hpp"
#include "MenuBar.hpp"
#include "ge/Object3D.hpp"
#include "core/Camera.hpp"
#include "core/SceneRenderer.hpp"
#include "core/WindowGLFW.hpp"
#include "core/ObjectChangeInfo.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

static std::string shading_mode_to_str(fury::Object3D::ShadingMode mode)
{
  switch (mode)
  {
  case fury::Object3D::ShadingMode::SMOOTH_SHADING:
    return "Smooth shading";
  case fury::Object3D::ShadingMode::FLAT_SHADING:
    return "Flat shading";
  case fury::Object3D::ShadingMode::NO_SHADING:
    return "No shading";
  default:
    return "Unknown";
  }
}

namespace fury
{
  SceneInfo::SceneInfo(SceneRenderer* scene) : UiComponent(scene)
  {
    KeyboardHandler* kh = static_cast<KeyboardHandler*>(scene->get_window()->get_input_handler(UserInputHandler::HandlerType::KEYBOARD));
    kh->on_key_state_change += new InstanceListener(this, &SceneInfo::handle_key_press);
  }

  void SceneInfo::tick()
  {
    if (!is_visible())
      return;

    WindowGLFW* window = m_scene->get_window();

    // stick menu to the right side of window
    const ImVec2 sz = ImVec2(window->width(), window->height());
    constexpr float scale_factor = 0.2f;
    MenuBar* menubar_component = static_cast<MenuBar*>(m_scene->get_ui().get_component("MenuBar"));
    ImGui::SetNextWindowSize(ImVec2(sz.x * scale_factor, sz.y - menubar_component->get_size().y));
    ImGui::SetNextWindowPos(ImVec2(sz.x - sz.x * scale_factor, menubar_component->get_size().y));
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
    if (ImGui::CollapsingHeader("General"))
    {
      if (ImGui::Checkbox("Fill polygons", &m_fill_polygons))
      {
        on_polygon_mode_change.notify(m_fill_polygons ? GL_FILL : GL_LINE);
      }
      if (ImGui::Checkbox("Show scene's bbox", &m_show_scene_bbox))
      {
        on_show_scene_bbox.notify(m_show_scene_bbox);
      }
    }

    if (!m_scene->get_selected_objects().empty())
    {
      render_object_properties(*m_scene->get_selected_objects().back());
    }

    ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  }

  void SceneInfo::render_object_properties(Object3D& drawable)
  {
    ImGui::SetNextItemOpen(true, ImGuiCond_::ImGuiCond_Once);
    const std::string& name = drawable.get_name();
    if (ImGui::TreeNode(name.c_str()))
    {
      // translation
      ImGui::Separator();
      ImGui::Text("Translation");
      ImGui::PushItemWidth(-1);
      const float item_width = ImGui::CalcItemWidth() - 5; // -5 to leave some offset from right
      ImGui::PopItemWidth();
      const float offset_from_left = ImGui::GetStyle().IndentSpacing;
      const float win_space_for_items_x = ImGui::GetWindowSize().x - offset_from_left;

      render_xyz_markers(offset_from_left, win_space_for_items_x);
      ImGui::PushItemWidth(-1);
      m_obj_translation = drawable.translation();
      glm::vec3 old_translation = drawable.translation();
      if (ImGui::InputFloat3("##translationLabel", &m_obj_translation.x))
      {
        drawable.translate(m_obj_translation - old_translation);
      }
      ImGui::PopItemWidth();
      ImGui::Dummy(ImVec2(0.f, 5.f));
      ImGui::GetStyle().ItemSpacing.x = 3.f;
      ImGui::PushItemWidth(item_width / 3);

      ObjectChangeInfo transformation_change;
      transformation_change.is_transformation_change = true;
      if (ImGui::SliderFloat("##X", &m_obj_translation.x, -50.0f, 50.0f))
      {
        drawable.translate(glm::vec3(m_obj_translation.x - old_translation.x, 0.f, 0.f));
        on_object_change.notify(&drawable, transformation_change);
      }

      ImGui::SameLine();
      if (ImGui::SliderFloat("##Y", &m_obj_translation.y, -50.0f, 50.0f))
      {
        drawable.translate(glm::vec3(0.f, m_obj_translation.y - old_translation.y, 0.f));
        on_object_change.notify(&drawable, transformation_change);
      }

      ImGui::SameLine();
      if (ImGui::SliderFloat("##Z", &m_obj_translation.z, -50.0f, 50.0f))
      {
        drawable.translate(glm::vec3(0.f, 0.f, m_obj_translation.z - old_translation.z));
        on_object_change.notify(&drawable, transformation_change);
      }

      // scale
      ImGui::Separator();
      ImGui::Text("Scale");
      render_xyz_markers(offset_from_left, win_space_for_items_x);
      ImGui::PushItemWidth(-1);
      glm::vec3 scale = drawable.scale();
      m_obj_scale = drawable.scale();
      if (ImGui::InputFloat3("##scaleLabel", &scale.x))
      {
        if (scale.x > 0 && scale.y > 0 && scale.z > 0)
          drawable.scale(scale);
      }
      ImGui::PopItemWidth();
      ImGui::Dummy(ImVec2(0, 5));
      ImGui::PushItemWidth(item_width / 3);

      if (ImGui::SliderFloat("##X2", &m_obj_scale.x, 0.1f, 3.f))
      {
        drawable.scale(m_obj_scale);
        on_object_change.notify(&drawable, transformation_change);
      }

      ImGui::SameLine();
      if (ImGui::SliderFloat("##Y2", &m_obj_scale.y, 0.1f, 3.f))
      {
        drawable.scale(m_obj_scale);
        on_object_change.notify(&drawable, transformation_change);
      }

      ImGui::SameLine();
      if (ImGui::SliderFloat("##Z2", &m_obj_scale.z, 0.1f, 3.f))
      {
        drawable.scale(m_obj_scale);
        on_object_change.notify(&drawable, transformation_change);
      }
      ImGui::PopItemWidth();

      // rotation
      ImGui::Separator();
      ImGui::Text("Rotation");
      bool rotating = drawable.is_rotating();
      float old_rot_angle = drawable.rotation_angle();
      m_obj_rotation_angle = old_rot_angle;
      m_obj_rotation_axis = drawable.rotation_axis();

      // TODO: rework rotations
      if (ImGui::Checkbox("Rotate every frame", &rotating))
        drawable.rotating(rotating);

      if (ImGui::SliderFloat("Angle(deg)", &m_obj_rotation_angle, -360.f, 360.f))
      {
        if (!drawable.is_rotating())
          drawable.rotate(m_obj_rotation_angle - old_rot_angle, m_obj_rotation_axis);
      }

      render_xyz_markers(offset_from_left, win_space_for_items_x);
      if (ImGui::SliderFloat("##X3", &m_obj_rotation_axis.x, 0.f, 1.f))
      {
        if (!drawable.is_rotating())
        {
          drawable.rotate(m_obj_rotation_angle, m_obj_rotation_axis);
          // TODO: set in SceneRenderer::tick() as well
          on_object_change.notify(&drawable, transformation_change);
        }
      }

      ImGui::SameLine();
      if (ImGui::SliderFloat("##Y3", &m_obj_rotation_axis.y, 0.f, 1.f))
      {
        if (!drawable.is_rotating())
        {
          drawable.rotate(m_obj_rotation_angle, m_obj_rotation_axis);
          // TODO: set in SceneRenderer::tick() as well
          on_object_change.notify(&drawable, transformation_change);
        }
      }

      ImGui::SameLine();
      if (ImGui::SliderFloat("##Z3", &m_obj_rotation_axis.z, 0.f, 1.f))
      {
        if (!drawable.is_rotating())
        {
          drawable.rotate(m_obj_rotation_angle, m_obj_rotation_axis);
          // TODO: set in SceneRenderer::tick() as well
          on_object_change.notify(&drawable, transformation_change);
        }
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
        if (same_line_cursor_pos.x + x_offset + width_of_next_checkbox > win_space_for_items_x)
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
          ImGui::SetNextItemWidth(win_space_for_items_x / 2);
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
          ImGui::Separator();
        }
      }
      ImGui::TreePop();
    }
  }

  void SceneInfo::render_xyz_markers(float offset_from_left, float width)
  {
    constexpr static const char* XYZ[] = { "X", "Y", "Z" };
    for (int i = 0; i < 3; i++)
    {
      // center labels over input fields
      ImGui::SetCursorPosX(offset_from_left + (width / 3 * i) + width / 6);
      ImGui::Text(XYZ[i]);
      if (i < 2)
        ImGui::SameLine();
    }
  }

  void SceneInfo::handle_key_press(KeyboardHandler::InputKey key, KeyboardHandler::KeyState state)
  {
    if (key == KeyboardHandler::InputKey::GRAVE_ACCENT && state == KeyboardHandler::KeyState::PRESSED)
    {
      m_is_visible = !m_is_visible;
      if (m_is_visible)
      {
        // disable any camera movement
        m_scene->get_camera().freeze();
        glfwSetInputMode(m_scene->get_window()->gl_window(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
      }
      else
      {
        m_scene->get_camera().unfreeze();
        glfwSetInputMode(m_scene->get_window()->gl_window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      }
    }
  }
}
