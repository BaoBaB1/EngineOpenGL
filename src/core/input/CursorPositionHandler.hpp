#pragma once

#include "UserInputHandler.hpp"
#include "core/Event.hpp"

namespace fury
{
  class CursorPositionHandler : public UserInputHandler
  {
  public:
    OnlyMovable(CursorPositionHandler)
    CursorPositionHandler(WindowGLFW* window);
    void update_current_pos(double x, double y) { m_cur_pos[0] = x, m_cur_pos[1] = y; }
    void update_ignore_frames() { m_ignore_frames = 3; }
    // new and old position
    Event<double, double, double, double> on_cursor_position_change;
  private:
    void callback(double xpos, double ypos);
    double m_prev_pos[2];
    double m_cur_pos[2];
    int m_ignore_frames = 0;
  };
}
