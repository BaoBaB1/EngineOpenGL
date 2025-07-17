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
    HandlerType get_type() const { return m_type; }
  protected:
    UserInputHandler(WindowGLFW* window, HandlerType type) : m_type(type), m_window(window) {}
  protected:
    HandlerType m_type = HandlerType::LAST_ITEM;
    WindowGLFW* m_window = nullptr;
  };
};
