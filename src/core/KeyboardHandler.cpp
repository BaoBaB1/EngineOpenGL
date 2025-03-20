#include "WindowGLFW.hpp"
#include "KeyboardHandler.hpp"
#include <cassert>

KeyboardHandler::KeyboardHandler(WindowGLFW* window) : UserInputHandler(window, HandlerType::KEYBOARD)
{
  auto key_callback = [](GLFWwindow* window, int key, int scancode, int action, int mods)
    {
      WindowGLFW* window_glfw = static_cast<WindowGLFW*>(glfwGetWindowUserPointer(window));
      static_cast<KeyboardHandler*>(window_glfw->get_input_handler(HandlerType::KEYBOARD))->key_callback(key, scancode, action, mods);
    };
  for (InputKey key : KeyboardHandler::registered_keys)
  {
    m_keystate[key] = KeyState::NO_STATE;
  }
  glfwSetKeyCallback(m_window->gl_window(), key_callback);
}

void KeyboardHandler::key_callback(int key, int scancode, int action, int mods)
{
  if (!disabled())
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
          m_pressed_keys.erase(m_pressed_key_map[ckey]);
          m_pressed_key_map.erase(ckey);
          on_key_state_change.notify(ckey, KeyState::RELEASED);
          break;
        case GLFW_PRESS:
          m_keystate[ckey] = KeyState::PRESSED;
          m_pressed_keys.push_back(ckey);
          m_pressed_key_map.emplace(std::make_pair(ckey, std::prev(m_pressed_keys.end())));
          on_key_state_change.notify(ckey, KeyState::PRESSED);
          break;
        }
      }
    }
  }
}

KeyboardHandler::KeyState KeyboardHandler::get_keystate(InputKey key) const
{
  return m_keystate.at(key);
}

void KeyboardHandler::disable()
{
  m_disabled = true;
  m_pressed_keys.clear();
  m_pressed_key_map.clear();
  for (const auto key : registered_keys)
  {
    reset_state(key);
  }
}

void KeyboardHandler::reset_state(InputKey key)
{
  m_keystate[key] = NO_STATE;
  on_key_state_change.notify(key, NO_STATE);
}
