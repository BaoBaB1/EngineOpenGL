#pragma once

#include "UserInputHandler.hpp"
#include "Event.hpp"

class MouseInputHandler : public UserInputHandler
{
public:
  OnlyMovable(MouseInputHandler)
  MouseInputHandler(MainWindow* window);
  Event<int, int, int> on_button_click;
private:
  void click_callback(GLFWwindow* window, int button, int action, int mods) const;
};
