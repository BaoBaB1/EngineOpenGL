#include "UserInputHandler.hpp"
#include "core/WindowGLFW.hpp"
#include <iostream>
#include <stdexcept>

static std::string handler_type_to_string(fury::UserInputHandler::HandlerType type)
{
  switch (type)
  {
  case fury::UserInputHandler::KEYBOARD:
    return "Keyboard input";
  case fury::UserInputHandler::CURSOR_POSITION:
    return "Cursor position";
  case fury::UserInputHandler::MOUSE_INPUT:
    return "Mouse input";
  default:
    return "Unknown";
  }
}

namespace fury
{
  UserInputHandler::UserInputHandler(WindowGLFW* window, HandlerType type)
  {
    if (window->get_input_handler(type))
    {
      std::string msg = handler_type_to_string(type) + " handler already exists\n";
      throw std::runtime_error(msg);
    }
    m_type = type;
    m_window = window;
    m_disabled = false;
  }

  void UserInputHandler::notify(bool _enable)
  {
    if (_enable)
      enable();
    else
      disable();
  }
};
