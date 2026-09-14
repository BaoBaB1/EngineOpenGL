#pragma once

#include "UiComponent.hpp"

namespace fury
{
  class Scene;

  struct OpenFileExplorerContext
  {
    bool save_scene = false;
    bool load_scene = false;
    bool import_asset = false;
    bool select_texture = false;
  };

  class MenuBar : public UiComponent
  {
  public:
    MenuBar() = default;
    void tick(float) override;
    void init(Scene* scene) override;
    const float get_height() const { return m_height; }
  private:
    float m_height = 0;
    float m_frame_padding = 7;
  };
} // namespace fury
