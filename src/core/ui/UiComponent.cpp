#include "UiComponent.hpp"
#include "core/Scene.hpp"

namespace fury
{
  void UiComponent::init(Scene* scene)
  {
    m_scene = scene;
  }

  void UiComponent::toggle_visibility()
  {
    if (m_is_visible)
    {
      hide();
    }
    else
    {
      show();
    }
  }

  void UiComponent::show()
  {
    if (!m_is_visible)
    {
      // Trigger only when state actually has been changed
      m_is_visible = true;
      on_show.notify();
    }
  }

  void UiComponent::hide()
  {
    if (m_is_visible)
    {
      // Trigger only when state actually has been changed
      m_is_visible = false;
      on_hide.notify();
    }
  }

} // namespace fury
