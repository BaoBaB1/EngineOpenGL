#pragma once

#include "WindowGLFW.hpp"
#include "Singleton.hpp"

namespace fury
{
  class Application : public Singleton<Application>
  {
  public:
    void run();
    ~Application();
  private:
    Application();
    friend class Singleton<Application>;
  private:
    WindowGLFW m_window;
  };
};
