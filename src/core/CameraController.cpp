#include "CameraController.hpp"
#include "input/InputSystem.hpp"
#include "Logger.hpp"
#include "Camera.hpp"

namespace fury
{
  void CameraController::tick(float dt)
  {
    m_camera->tick(dt);
    const InputSystem& input_system = InputSystem::instance();
    if (input_system.get_axis_value("MoveForward"))
    {
      m_camera->move(Camera::Direction::FORWARD);
    }
    if (input_system.get_axis_value("MoveBackward"))
    {
      m_camera->move(Camera::Direction::BACKWARD);
    }
    if (input_system.get_axis_value("MoveLeft"))
    {
      m_camera->move(Camera::Direction::LEFT);
    }
    if (input_system.get_axis_value("MoveRight"))
    {
      m_camera->move(Camera::Direction::RIGHT);
    }
    if (input_system.get_axis_value("MoveUp"))
    {
      m_camera->move(Camera::Direction::UP);
    }
    if (input_system.get_axis_value("MoveDown"))
    {
      m_camera->move(Camera::Direction::DOWN);
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
