#pragma once

#include "WindowGLFW.hpp"
#include "Singleton.hpp"
#include "Scene.hpp"

namespace fury
{
  class Application : public Singleton<Application>
  {
  public:
    void run();
    Scene& get_scene() { return m_scene; }
    ~Application();
  private:
    Application();
    friend class Singleton<Application>;
  private:
    WindowGLFW m_window;
    Scene m_scene;
  };
}; // namespace fury
