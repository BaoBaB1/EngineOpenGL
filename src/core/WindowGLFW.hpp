#pragma once

#include "Macros.hpp"
#include "Event.hpp"

struct GLFWwindow;

namespace fury
{
  class WindowGLFW
  {
  public:
    FURY_OnlyMovable(WindowGLFW)
    static void get_monitor_resolution(int& horizontal, int& vertical);
    WindowGLFW();
    ~WindowGLFW();
    GLFWwindow* gl_window() const { return m_window; }
    void set_width(int w) { m_width = w; }
    void set_height(int h) { m_height = h; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    const char* title() const { return m_title; }
    Event<int, int> window_size_changed;
    Event<bool> on_window_focus_changed;
    Event<bool> on_window_cursor_entered;
  private:
    const char* m_title = "MainWindow";
    int m_width = 1600;
    int m_height = 900;
    GLFWwindow* m_window;
  };
} // namespace fury
