#pragma once

#include "./utils/Macro.hpp"

namespace fury
{
  class WindowGLFW;

  class UserInputHandler
  {
  public:
    enum HandlerType
    {
      KEYBOARD,
      CURSOR_POSITION,
      MOUSE_INPUT,
      LAST_ITEM
    };
  public:
    OnlyMovable(UserInputHandler)
    virtual void enable() { m_disabled = false; }
    virtual void disable() { m_disabled = true; }
    bool disabled() const { return m_disabled; }
    HandlerType type() const { return m_type; }
  protected:
    UserInputHandler(WindowGLFW* window, HandlerType type) : m_type(type), m_window(window) {}
  protected:
    HandlerType m_type;
    WindowGLFW* m_window = nullptr;
    bool m_disabled = false;
  };
};
