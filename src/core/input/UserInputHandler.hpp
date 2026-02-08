#pragma once

#include "core/Macros.hpp"

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
    FURY_OnlyMovable(UserInputHandler)
    HandlerType get_type() const { return m_type; }
  protected:
    UserInputHandler(WindowGLFW* window, HandlerType type) : m_type(type), m_window(window) {}
  protected:
    HandlerType m_type = HandlerType::LAST_ITEM;
    WindowGLFW* m_window = nullptr;
  };
};
