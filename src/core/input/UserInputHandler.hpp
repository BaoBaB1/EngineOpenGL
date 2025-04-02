#pragma once

#include "./utils/IObserver.hpp"
#include "./utils/Macro.hpp"
#include <map>


//using namespace OpenGLEngineUtils;

namespace fury
{
  class WindowGLFW;

  class UserInputHandler : public furyutils::IObserver
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
  protected:
    UserInputHandler(WindowGLFW* window, HandlerType input_type);
  protected:
    HandlerType m_type;
    WindowGLFW* m_window = nullptr;
    bool m_disabled = false;
  };
};
