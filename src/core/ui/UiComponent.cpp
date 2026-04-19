#include "UiComponent.hpp"
#include "core/SceneRenderer.hpp"

namespace fury
{
  UiComponent::UiComponent(SceneRenderer* scene) : m_scene(scene)
  {
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
