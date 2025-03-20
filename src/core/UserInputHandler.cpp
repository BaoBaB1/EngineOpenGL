#include "UserInputHandler.hpp"
#include "WindowGLFW.hpp"
#include <iostream>
#include <stdexcept>

static std::string handler_type_to_string(UserInputHandler::HandlerType type)
{
  switch (type)
  {
  case UserInputHandler::KEYBOARD:
    return "Keyboard input";
  case UserInputHandler::CURSOR_POSITION:
    return "Cursor position";
  case UserInputHandler::MOUSE_INPUT:
    return "Mouse input";
  default:
    return "Unknown";
  }
}

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

UserInputHandler::~UserInputHandler()
{
}

void UserInputHandler::notify(bool _enable)
{
  if (_enable)
    enable();
  else
    disable();
}
