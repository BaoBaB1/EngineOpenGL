#include "CameraController.hpp"
#include "input/InputSystem.hpp"
#include "Logger.hpp"
#include "Camera.hpp"

namespace fury
{
  void CameraController::tick(float dt)
  {
    if (m_camera->is_freezed())
    {
      return;
    }
    const InputSystem& input_system = InputSystem::instance();
    float speed = m_camera->get_speed() * dt;
    glm::vec3 pos = m_camera->get_position();
    const glm::vec3& up = m_camera->get_up();
    const glm::vec3& right = m_camera->get_right();
    const glm::vec3& forward = m_camera->get_target();
    if (float scale = input_system.get_axis_value("SpeedUp"))
    {
      speed *= scale;
    }
    if (input_system.get_axis_value("MoveForward"))
    {
      pos += speed * forward;
    }
    if (input_system.get_axis_value("MoveBackward"))
    {
      pos -= speed * forward;
    }
    if (input_system.get_axis_value("MoveLeft"))
    {
      pos -= right * speed;
    }
    if (input_system.get_axis_value("MoveRight"))
    {
      pos += right * speed;
    }
    if (input_system.get_axis_value("MoveUp"))
    {
      pos += speed * up;
    }
    if (input_system.get_axis_value("MoveDown"))
    {
      pos -= speed * up;
    }

    // set position only if position actually changed, since
    // Camera::set_position internally updates dirty flag
    if (pos != m_camera->get_position())
    {
      m_camera->set_position(pos);
    }
  }

  CameraController::~CameraController()
  {
    InputSystem::instance().on_cursor_moved.remove_by_owner(this);
  }

  void CameraController::init(Camera* camera)
  {
    if (m_camera)
    {
      Logger::error("Camera controller already initialized.");
      return;
    }
    if (!camera)
    {
      Logger::error("Camera for camera controller is null");
      return;
    }
    m_camera = camera;
    InputSystem::instance().on_cursor_moved += new InstanceListener(this, &CameraController::handle_cursor_move);
  }

  void CameraController::handle_cursor_move(double newx, double newy, double oldx, double oldy)
  {
    const double dx = newx - oldx;
    const double dy = newy - oldy;
    if (dx != 0.0 || dy != 0.0)
    {
      m_camera->add_to_yaw_and_pitch(dx, dy);
    }
  }
} // namespace fury
