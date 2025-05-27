#include "Gizmo.hpp"
#include "core/input/KeyboardHandler.hpp"
#include "core/Camera.hpp"
#include "core/ObjectChangeInfo.hpp"
#include "core/SceneRenderer.hpp"
#include "core/WindowGLFW.hpp"
#include "ge/Object3D.hpp"

#include <ImGuizmo.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace fury
{
  Gizmo::Gizmo(SceneRenderer* scene) : UiComponent(scene)
  {
    m_guizmo_operation = ImGuizmo::OPERATION::TRANSLATE;
  }

  void Gizmo::tick()
  {
    // Gizmo
    if (!m_scene->get_selected_objects().empty() && m_scene->get_ui().get_component("SceneInfo")->is_visible())
    {
      WindowGLFW* window = m_scene->get_window();
      KeyboardHandler* kh = window->get_input_handler<KeyboardHandler>(UserInputHandler::KEYBOARD);
      // default mode is translation
      if (kh->get_keystate(KeyboardHandler::InputKey::T) == KeyboardHandler::KeyState::PRESSED)
      {
        m_guizmo_operation = ImGuizmo::OPERATION::TRANSLATE;
      }
      else if (kh->get_keystate(KeyboardHandler::InputKey::R) == KeyboardHandler::KeyState::PRESSED)
      {
        m_guizmo_operation = ImGuizmo::OPERATION::ROTATE;
      }
      else if (kh->get_keystate(KeyboardHandler::InputKey::S) == KeyboardHandler::KeyState::PRESSED)
      {
        m_guizmo_operation = ImGuizmo::OPERATION::SCALE;
      }

      ImGuizmo::BeginFrame();
      ImGuizmo::SetOrthographic(false);
      ImGuizmo::SetRect(0, 0, window->width(), window->height());

      Object3D* obj = m_scene->get_selected_objects().back();
      Camera& cam = m_scene->get_camera();
      ImGuizmo::MODE gizmo_mode = ImGuizmo::MODE::LOCAL;

      glm::mat4 model_mat = obj->model_matrix();
      ImGuizmo::Manipulate(glm::value_ptr(cam.view_matrix()), glm::value_ptr(m_scene->get_camera().get_projection_matrix()),
        static_cast<ImGuizmo::OPERATION>(m_guizmo_operation), gizmo_mode, glm::value_ptr(model_mat));
      if (ImGuizmo::IsUsing() && obj->model_matrix() != model_mat)
      {
        obj->model_matrix() = model_mat;
        ObjectChangeInfo info;
        info.is_transformation_change = true;
        on_object_change.notify(obj, info);
      }
    }
  }
}
