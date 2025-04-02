#include <GLFW/glfw3.h>
#include "MouseInputHandler.hpp"
#include "core/WindowGLFW.hpp"
#include <cassert>

namespace fury
{
  MouseInputHandler::MouseInputHandler(WindowGLFW* window) : UserInputHandler(window, HandlerType::MOUSE_INPUT)
  {
    auto click_callback = [](GLFWwindow* window, int button, int action, int mods)
      {
        WindowGLFW* window_glfw = static_cast<WindowGLFW*>(glfwGetWindowUserPointer(window));
        static_cast<MouseInputHandler*>(window_glfw->get_input_handler(HandlerType::MOUSE_INPUT))->click_callback(window, button, action, mods);
      };
    glfwSetMouseButtonCallback(m_window->gl_window(), click_callback);
  }

  void MouseInputHandler::click_callback(GLFWwindow* window, int button, int action, int mods) const
  {
    if (!disabled())
    {
      if (action == GLFW_PRESS)
      {
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        on_button_click.notify(button, static_cast<int>(x), static_cast<int>(y));
      }
    }
  }
}
