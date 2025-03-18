#include "MouseInputHandler.hpp"
#include "MainWindow.hpp"

MouseInputHandler::MouseInputHandler(MainWindow* window) : UserInputHandler(window, HandlerType::MOUSE_INPUT)
{
  auto click_callback = [](GLFWwindow* window, int button, int action, int mods)
    {
      // this must point to MainWindow instance
      void* old_ptr = glfwGetWindowUserPointer(window);
      assert(old_ptr);
      glfwSetWindowUserPointer(window, m_ptrs[HandlerType::MOUSE_INPUT]);
      static_cast<MouseInputHandler*>(glfwGetWindowUserPointer(window))->click_callback(window, button, action, mods);
      glfwSetWindowUserPointer(window, old_ptr);
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
