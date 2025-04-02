#pragma once

#include "UserInputHandler.hpp"
#include "core/Event.hpp"

struct GLFWwindow;

namespace fury
{
  class MouseInputHandler : public UserInputHandler
  {
  public:
    OnlyMovable(MouseInputHandler)
    MouseInputHandler(WindowGLFW* window);
    Event<int, int, int> on_button_click;
  private:
    void click_callback(GLFWwindow* window, int button, int action, int mods) const;
  };
};
