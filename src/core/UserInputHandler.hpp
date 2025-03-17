#pragma once

#include <glad/glad.h>
#include <glm/vec3.hpp>
#include <GLFW/glfw3.h>
#include <map>
#include "Debug.hpp"
#include "./utils/IObserver.hpp"
#include "./utils/Macro.hpp"

class MainWindow;

using namespace OpenGLEngineUtils;

class UserInputHandler : public IObserver
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
  void notify(bool _enable) override;
  bool disabled() const { return m_disabled; }
  HandlerType type() const { return m_type; }
  ~UserInputHandler();
protected:
  UserInputHandler(MainWindow* window, HandlerType input_type);
protected:
  HandlerType m_type;
  MainWindow* m_window;
  bool m_disabled;
  static std::map<HandlerType, void*> m_ptrs; // for correct cast in callbacks for glfwSetWindowUserPointer
};
