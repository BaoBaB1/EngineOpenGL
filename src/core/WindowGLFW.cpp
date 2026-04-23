#include "WindowGLFW.hpp"
#include "Logger.hpp"
#include "input/InputSystem.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace fury
{
  void WindowGLFW::get_monitor_resolution(int& horizontal, int& vertical)
  {
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    auto info = glfwGetVideoMode(monitor);
    horizontal = info->width;
    vertical = info->height;
  }

  WindowGLFW::WindowGLFW(int width, int height, const char* title)
  {
    init(width, height, title);
  }

  void WindowGLFW::init(int width, int height, const char* title)
  {
    m_width = width;
    m_height = height;
    m_title = title;
    // Tell GLFW what version of OpenGL we are using
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    // Tell GLFW we are using the CORE profile (only modern functions)
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    m_window = glfwCreateWindow(width, height, title, nullptr, glfwGetCurrentContext());
    if (m_window == nullptr)
    {
      Logger::critical("Failed to create GLFW window. Make sure your driver supports at least OpenGL 4.4");
      return;
    }
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPos(m_window, m_width / 2., m_height / 2.);
    glfwMakeContextCurrent(m_window);
    // disable vsync
    glfwSwapInterval(0);
    glfwSetWindowSizeLimits(m_window, 1600, 900, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwSetWindowUserPointer(m_window, this);
    auto window_size_change_callback = [](GLFWwindow* window, int width, int height)
    {
      static_cast<WindowGLFW*>(glfwGetWindowUserPointer(window))->on_window_size_change.notify(width, height);
    };
    auto window_focus_change_callback = [](GLFWwindow* window, int focused)
    {
      static_cast<WindowGLFW*>(glfwGetWindowUserPointer(window))->on_window_focus_changed.notify(focused);
    };
    auto window_cursor_enter_callback = [](GLFWwindow* window, int entered)
    {
      static_cast<WindowGLFW*>(glfwGetWindowUserPointer(window))->on_window_cursor_entered.notify(entered);
    };
    glfwSetWindowSizeCallback(m_window, window_size_change_callback);
    glfwSetWindowFocusCallback(m_window, window_focus_change_callback);
    glfwSetCursorEnterCallback(m_window, window_cursor_enter_callback);
    gladLoadGL();
    glViewport(0, 0, m_width, m_height);
  }

  WindowGLFW::~WindowGLFW()
  {
    glfwDestroyWindow(m_window);
  }
} // namespace fury
