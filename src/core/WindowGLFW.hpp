#pragma once

#include "Macros.hpp"
#include "input/UserInputHandler.hpp"
#include "Event.hpp"
#include <map>
#include <memory>

struct GLFWwindow;

namespace fury
{
  class WindowGLFW
  {
  public:
    FURY_OnlyMovable(WindowGLFW)
    static void get_monitor_resolution(int& horizontal, int& vertical);
    WindowGLFW() = default;
    WindowGLFW(int width, int height, const char* title);
    ~WindowGLFW();
    void init(int width, int height, const char* title);
    GLFWwindow* gl_window() const { return m_window; }
    template<typename T>
    T* get_input_handler(UserInputHandler::HandlerType type);
    void set_width(int w) { m_width = w; }
    void set_height(int h) { m_height = h; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    const char* title() const { return m_title; }
    Event<int, int> on_window_size_change;
  private:
    const char* m_title;
    int m_width;
    int m_height;
    GLFWwindow* m_window;
    std::map<UserInputHandler::HandlerType, std::unique_ptr<UserInputHandler>> m_input_handlers;
  };

  template<typename T>
  T* WindowGLFW::get_input_handler(UserInputHandler::HandlerType type)
  {
    auto it = m_input_handlers.find(type);
    return (it != m_input_handlers.end()) ? static_cast<T*>(it->second.get()) : nullptr;
  }
}
