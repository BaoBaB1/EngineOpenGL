#pragma once

#include "core/ITickable.hpp"
#include "core/Event.hpp"
#include <string>

namespace fury
{
  class Scene;

  class UiComponent : public ITickable
  {
  public:
    UiComponent() = default;
    virtual void show();
    virtual void hide();
    virtual void init(Scene* scene);
    void toggle_visibility();
    bool is_visible() const { return m_is_visible; }
    template<typename T>
    T* as()
    {
      return static_cast<T*>(this);
    }
    Event<> on_show;
    Event<> on_hide;
  protected:
    bool m_is_visible = false;
    Scene* m_scene = nullptr;
    std::string m_name;
  };
} // namespace fury
