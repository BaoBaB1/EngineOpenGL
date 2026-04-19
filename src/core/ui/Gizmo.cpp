#include "Gizmo.hpp"
#include "core/Camera.hpp"
#include "core/ObjectChangeInfo.hpp"
#include "core/SceneRenderer.hpp"
#include "core/WindowGLFW.hpp"
#include "ge/Object3D.hpp"
#include "core/SceneGraphManager.hpp"

#include <imgui.h>
#include <ImGuizmo.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace fury
{
  Gizmo::Gizmo(SceneRenderer* scene) : UiComponent(scene)
  {
    m_gizmo_operation = ImGuizmo::OPERATION::TRANSLATE;
    auto gizmo_mode_change_reactor = [this](InputCode key)
    {
      const InputSystem& input_system = InputSystem::instance();
      if (input_system.get_active_input_context()->name == "UI")
      {
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
      }
    };

    InputSystem::instance().on_keyboard_button_clicked +=
        new FunctionListener(std::function(gizmo_mode_change_reactor));
  }

  void Gizmo::tick(float)
  {
    // Gizmo
    if (!m_scene->get_selected_objects().empty() && m_scene->get_ui().get_component("SceneInfo")->is_visible())
    {
      WindowGLFW* window = m_scene->get_window();
      ImGuizmo::BeginFrame();
      ImGuizmo::SetOrthographic(false);
      ImGuizmo::SetRect(0, 0, window->width(), window->height());

      Object3D* obj = m_scene->get_selected_objects().back();
      Camera& cam = m_scene->get_camera();
      ImGuizmo::MODE gizmo_mode = ImGuizmo::MODE::LOCAL;

      auto node = SceneGraphManager::get_entity_node<TransformationSceneNode>(obj->get_id());
      glm::mat4 model_mat = node->get_world_mat();
      ImGuizmo::Manipulate(glm::value_ptr(cam.get_view_matrix()),
                           glm::value_ptr(m_scene->get_camera().get_projection_matrix()),
                           static_cast<ImGuizmo::OPERATION>(m_gizmo_operation), gizmo_mode, glm::value_ptr(model_mat));
      if (ImGuizmo::IsUsing() && node->get_world_mat() != model_mat)
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
        node->set_scale(new_scale);
        node->set_translation(new_translation);
        node->set_rotation(new_rotation);
      }
    }
  }
} // namespace fury
