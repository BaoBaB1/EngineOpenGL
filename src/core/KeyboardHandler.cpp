#include "MainWindow.hpp"
#include "KeyboardHandler.hpp"

KeyboardHandler::KeyboardHandler(MainWindow* window) : UserInputHandler(window, HandlerType::KEYBOARD)
{
  auto key_callback = [](GLFWwindow* window, int key, int scancode, int action, int mods)
    {
      glfwSetWindowUserPointer(window, m_ptrs[HandlerType::KEYBOARD]);
      static_cast<KeyboardHandler*>(glfwGetWindowUserPointer(window))->key_callback(key, scancode, action, mods);
    };
  for (InputKey key : KeyboardHandler::registered_keys)
  {
    m_keystate[key] = KeyState::NO_STATE;
  }
  glfwSetKeyCallback(m_window->gl_window(), key_callback);
}

void KeyboardHandler::key_callback(int key, int scancode, int action, int mods)
{
  for (auto reg_key : KeyboardHandler::registered_keys)
  {
    if (reg_key == key)
    {
      InputKey ckey = static_cast<InputKey>(key);
      switch (action)
      {
      case GLFW_RELEASE:
        m_keystate[ckey] = KeyState::RELEASED;
        break;
      case GLFW_PRESS:
        m_keystate[ckey] = KeyState::PRESSED;
        break;
      }
    }
  }
}

KeyboardHandler::KeyState KeyboardHandler::get_keystate(InputKey key) const
{
  return m_keystate.at(key);
}

void KeyboardHandler::reset_state(InputKey key)
{
  m_keystate[key] = NO_STATE;
}
