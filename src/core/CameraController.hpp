#pragma once

#include "ITickable.hpp"
#include "Macros.hpp"

namespace fury
{
  class KeyboardHandler;
  class CursorPositionHandler;
  class Camera;

  class CameraController : public ITickable
  {
  public:
    FURY_OnlyMovable(CameraController)
    CameraController() = default;
    ~CameraController();
    void init(Camera* camera, KeyboardHandler* keyboard_handler, CursorPositionHandler* cursor_handler);
    void tick(float dt) override;
  private:
    void handle_cursor_move(double newx, double newy, double oldx, double oldy);
  private:
    // from where we get input callbacks
    CursorPositionHandler* m_cursor_handler = nullptr;
    KeyboardHandler* m_keyboard_handler = nullptr;
    Camera* m_camera = nullptr;
  };
}
