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
    void init(Camera* camera);
    void tick(float dt) override;
  private:
    void handle_cursor_move(double newx, double newy, double oldx, double oldy);
  private:
    Camera* m_camera = nullptr;
  };
}
