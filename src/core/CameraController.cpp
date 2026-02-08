#include "CameraController.hpp"
#include "input/CursorPositionHandler.hpp"
#include "input/KeyboardHandler.hpp"
#include "Logger.hpp"
#include "Camera.hpp"

namespace fury
{
  void CameraController::tick(float dt)
  {
    m_camera->tick(dt);
    // do movement every frame while movement key is in PRESSED state
    if (m_keyboard_handler && !m_keyboard_handler->get_pressed_keys().empty())
    {
      using Key = KeyboardHandler::InputKey;
      for (Key key : m_keyboard_handler->get_pressed_keys())
      {
        switch (key)
        {
        case Key::W:
        case Key::ARROW_UP:
          m_camera->move(Camera::Direction::FORWARD);
          break;
        case Key::A:
        case Key::ARROW_LEFT:
          m_camera->move(Camera::Direction::LEFT);
          break;
        case Key::S:
        case Key::ARROW_DOWN:
          m_camera->move(Camera::Direction::BACKWARD);
          break;
        case Key::D:
        case Key::ARROW_RIGHT:
          m_camera->move(Camera::Direction::RIGHT);
          break;
        case Key::SPACE:
          m_camera->move(Camera::Direction::UP);
          break;
        case Key::LEFT_CTRL:
          m_camera->move(Camera::Direction::DOWN);
          break;
        }
      }
    }
  }

  CameraController::~CameraController()
  {
    m_cursor_handler->on_cursor_position_change.remove_by_owner(this);
  }

  void CameraController::init(Camera* camera, KeyboardHandler* keyboard_handler, CursorPositionHandler* cursor_handler)
  {
    if (m_camera || m_cursor_handler || m_keyboard_handler)
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
    m_keyboard_handler = keyboard_handler;
    m_cursor_handler = cursor_handler;
    if (!keyboard_handler)
    {
      Logger::warn("Camera controller is missing keyboard handler");
    }
    if (!cursor_handler)
    {
      Logger::warn("Camera controller is missing cursor position handler");
    }
    else
    {
      cursor_handler->on_cursor_position_change += new InstanceListener(this, &CameraController::handle_cursor_move);
    }
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
}
