#pragma once

#include "core/ITickable.hpp"
#include "core/Event.hpp"

namespace fury
{
  class SceneRenderer;

  class UiComponent : public ITickable
  {
  public:
    UiComponent(SceneRenderer* scene);
    virtual void show();
    virtual void hide();
    bool is_visible() const { return m_is_visible; }
    Event<> on_show;
    Event<> on_hide;
  protected:
    bool m_is_visible = false;
    SceneRenderer* m_scene = nullptr;
  };
} // namespace fury
