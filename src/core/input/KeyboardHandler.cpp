#include "core/WindowGLFW.hpp"
#include "KeyboardHandler.hpp"
#include <GLFW/glfw3.h>
#include <cassert>

namespace
{
  using namespace fury;
  // glfw key code -> InputKey
  std::map<int, KeyboardHandler::InputKey> mapped_keys =
  {
    {GLFW_KEY_W, KeyboardHandler::InputKey::W},
    {GLFW_KEY_A, KeyboardHandler::InputKey::A},
    {GLFW_KEY_S, KeyboardHandler::InputKey::S},
    {GLFW_KEY_D, KeyboardHandler::InputKey::D},
    {GLFW_KEY_T, KeyboardHandler::InputKey::T},
    {GLFW_KEY_R, KeyboardHandler::InputKey::R},
    {GLFW_KEY_UP, KeyboardHandler::InputKey::ARROW_UP},
    {GLFW_KEY_DOWN, KeyboardHandler::InputKey::ARROW_DOWN},
    {GLFW_KEY_LEFT, KeyboardHandler::InputKey::ARROW_LEFT},
    {GLFW_KEY_RIGHT, KeyboardHandler::InputKey::ARROW_RIGHT},
    {GLFW_KEY_LEFT_SHIFT, KeyboardHandler::InputKey::LEFT_SHIFT},
    {GLFW_KEY_SPACE, KeyboardHandler::InputKey::SPACE},
    {GLFW_KEY_LEFT_CONTROL, KeyboardHandler::InputKey::LEFT_CTRL},
    {GLFW_KEY_ESCAPE, KeyboardHandler::InputKey::ESC},
    {GLFW_KEY_GRAVE_ACCENT, KeyboardHandler::InputKey::GRAVE_ACCENT},
    {GLFW_KEY_O, KeyboardHandler::InputKey::O},
    {GLFW_KEY_P, KeyboardHandler::InputKey::P},
    {GLFW_KEY_L, KeyboardHandler::InputKey::L},
    {GLFW_KEY_BACKSPACE, KeyboardHandler::InputKey::BACKSPACE}
  };
}

namespace fury
{
  KeyboardHandler::KeyboardHandler(WindowGLFW* window) : UserInputHandler(window, HandlerType::KEYBOARD)
  {
    auto key_callback = [](GLFWwindow* window, int key, int scancode, int action, int mods)
      {
        WindowGLFW* window_glfw = static_cast<WindowGLFW*>(glfwGetWindowUserPointer(window));
        static_cast<KeyboardHandler*>(window_glfw->get_input_handler(HandlerType::KEYBOARD))->key_callback(key, scancode, action, mods);
      };
    for (int i = 0; i < static_cast<int>(InputKey::LAST); i++)
    {
      m_keystate[static_cast<InputKey>(i)] = KeyState::RELEASED;
    }
    glfwSetKeyCallback(m_window->gl_window(), key_callback);
  }

  void KeyboardHandler::key_callback(int key, int scancode, int action, int mods)
  {
    if (!disabled())
    {
      auto iter = ::mapped_keys.find(key);
      if (iter != ::mapped_keys.end())
      {
        InputKey ckey = iter->second;
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

  KeyboardHandler::KeyState KeyboardHandler::get_keystate(InputKey key) const
  {
    return m_keystate.at(key);
  }

  void KeyboardHandler::disable()
  {
    m_disabled = true;
    m_pressed_keys.clear();
    m_pressed_key_map.clear();
    for (int i = 0; i < static_cast<int>(InputKey::LAST); i++)
    {
      m_keystate[static_cast<InputKey>(i)] = RELEASED;
    }
  }
};
