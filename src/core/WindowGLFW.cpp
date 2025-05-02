#include "WindowGLFW.hpp"
#include "input/KeyboardHandler.hpp"
#include "input/CursorPositionHandler.hpp"
#include "input/MouseInputHandler.hpp"
#include "Debug.hpp"
#include "Logger.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

static void window_focus_callback(GLFWwindow* window, int focused)
{
  if (focused)
  {
    fury::WindowGLFW* winglfw = static_cast<fury::WindowGLFW*>(glfwGetWindowUserPointer(window));
    fury::CursorPositionHandler* cp = static_cast<fury::CursorPositionHandler*>(winglfw->get_input_handler(fury::UserInputHandler::CURSOR_POSITION));
    cp->update_ignore_frames();
  }
}

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
    m_window = glfwCreateWindow(width, height, title, nullptr, glfwGetCurrentContext());
    if (m_window == nullptr) {
      Logger::critical("Failed to create GLFW window");
      return;
    }
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPos(m_window, m_width / 2., m_height / 2.);
    m_input_handlers[UserInputHandler::HandlerType::KEYBOARD] = std::make_unique<KeyboardHandler>(this);
    m_input_handlers[UserInputHandler::HandlerType::CURSOR_POSITION] = std::make_unique<CursorPositionHandler>(this);
    m_input_handlers[UserInputHandler::HandlerType::MOUSE_INPUT] = std::make_unique<MouseInputHandler>(this);
    glfwMakeContextCurrent(m_window);
    // disable vsync
    glfwSwapInterval(0);
    glfwSetWindowSizeLimits(m_window, 1600, 900, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwSetWindowFocusCallback(m_window, window_focus_callback);

    glfwSetWindowUserPointer(m_window, this);
    auto window_size_change_callback = [](GLFWwindow* window, int width, int height)
      {
        static_cast<WindowGLFW*>(glfwGetWindowUserPointer(window))->on_window_size_change.notify(width, height);
      };
    glfwSetWindowSizeCallback(m_window, window_size_change_callback);
    gladLoadGL();
    glViewport(0, 0, m_width, m_height);
  }

  UserInputHandler* WindowGLFW::get_input_handler(UserInputHandler::HandlerType type)
  {
    auto it = m_input_handlers.find(type);
    return (it != m_input_handlers.end()) ? it->second.get() : nullptr;
  }

  WindowGLFW::~WindowGLFW()
  {
    glfwDestroyWindow(m_window);
  }
}
