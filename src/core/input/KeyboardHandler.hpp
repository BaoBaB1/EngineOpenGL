#pragma once

#include "UserInputHandler.hpp"
#include "core/Event.hpp"
#include <list>
#include <map>

namespace fury
{
  class KeyboardHandler : public UserInputHandler
  {
  public:
    enum InputKey : uint16_t
    {
      W, A, S, D, T, R, O, P, L,
      ARROW_UP,
      ARROW_DOWN,
      ARROW_LEFT,
      ARROW_RIGHT,
      LEFT_SHIFT,
      SPACE,
      LEFT_CTRL,
      ESC,
      GRAVE_ACCENT, // tilde
      BACKSPACE,
      LAST
    };

    enum KeyState
    {
      PRESSED,
      RELEASED,
    };

    OnlyMovable(KeyboardHandler)
    KeyboardHandler(WindowGLFW* window);
    KeyboardHandler::KeyState get_keystate(KeyboardHandler::InputKey key) const;
    void disable() override;
    const std::list<InputKey>& get_pressed_keys() const { return m_pressed_keys; }
    Event<InputKey, KeyState> on_key_state_change;
  private:
    void key_callback(int key, int scancode, int action, int mods);
    // key, state
    std::map<InputKey, KeyState> m_keystate;
    std::list<InputKey> m_pressed_keys;
    std::map<InputKey, decltype(m_pressed_keys)::const_iterator> m_pressed_key_map;
  };
};
