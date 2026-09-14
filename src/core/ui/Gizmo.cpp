#include "Gizmo.hpp"
#include "core/Camera.hpp"
#include "core/Scene.hpp"
#include "core/WindowGLFW.hpp"
#include "core/ecs/components/TransformComponent.hpp"
#include "core/ecs/components/DirtyEntityTagComponent.hpp"

#include <imgui.h>
#include <ImGuizmo.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace fury
{
  Gizmo::Gizmo()
  {
    m_gizmo_operation = ImGuizmo::OPERATION::TRANSLATE;
    auto gizmo_mode_change_reactor = [this](InputCode key)
    {
      if (!m_is_visible)
      {
        return;
      }
      if (key == InputCode::FURY_KEY_T)
      {
        m_gizmo_operation = ImGuizmo::OPERATION::TRANSLATE;
      }
      else if (key == InputCode::FURY_KEY_S)
      {
        m_gizmo_operation = ImGuizmo::OPERATION::SCALE;
      }
      else if (key == InputCode::FURY_KEY_R)
      {
        m_gizmo_operation = ImGuizmo::OPERATION::ROTATE;
      }
    };

    InputSystem::instance().on_keyboard_button_clicked +=
        new FunctionListener(std::function(gizmo_mode_change_reactor));
  }

  void Gizmo::tick(float)
  {
    if (!m_is_visible || m_scene->get_selected_entity() == g_invalid_entity_id)
    {
      return;
    }

    WindowGLFW* window = m_scene->get_window();
    ImGuizmo::BeginFrame();
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetRect(0, 0, window->width(), window->height());

    auto& transform =
        m_scene->get_component_manager().get_component<ecs::TransformComponent>(m_scene->get_selected_entity());
    const Camera& cam = m_scene->get_camera();
    ImGuizmo::MODE gizmo_mode = ImGuizmo::MODE::LOCAL;

    glm::mat4 model_mat = transform.transform_node.get_world_mat();
    glm::mat4 vm = cam.get_view_matrix();
    ImGuizmo::Manipulate(glm::value_ptr(vm), glm::value_ptr(m_scene->get_camera().get_projection_matrix()),
                         static_cast<ImGuizmo::OPERATION>(m_gizmo_operation), gizmo_mode, glm::value_ptr(model_mat));
    if (ImGuizmo::IsUsing() && transform.transform_node.get_world_mat() != model_mat)
    {
      glm::vec3 new_scale;
      glm::quat new_rotation;
      glm::vec3 new_translation;
      glm::vec3 new_skew;
      glm::vec4 new_perspective;
      // TODO: custom decompose without skew and perspective
      // can be false e.g. with small scale values ...
      if (!glm::decompose(model_mat, new_scale, new_rotation, new_translation, new_skew, new_perspective))
      {
        return;
      }
      transform.transform_node.set_scale(new_scale);
      transform.transform_node.set_translation(new_translation);
      transform.transform_node.set_rotation(new_rotation);
      auto& dirty_tag = m_scene->get_component_manager().get_or_create_component<ecs::DirtyEntityTagComponent>(
          m_scene->get_selected_entity());
      dirty_tag.dirty_bits.transform_changed = true;
    }
  }
} // namespace fury
