#pragma once

#include "core/Macros.hpp"
#include "core/ITickable.hpp"
#include "SceneInfoWindow.hpp"
#include "MenuBar.hpp"
#include "FileExplorer.hpp"
#include "Gizmo.hpp"
#include "ItemSelectionWheel.hpp"
#include <memory>
#include <vector>
#include <string_view>

namespace fury
{
  class Scene;
  class UiComponent;

  class Ui : public ITickable
  {
  public:
    Ui() = default;
    FURY_OnlyMovable(Ui)
    void init(Scene* scene);
    void tick(float dt) override;
    const auto& get_components() const { return m_components; }
    Gizmo& get_gizmo() { return m_gizmo; }
    ItemSelectionWheel& get_item_selection_wheel() { return m_item_selection_wheel; }
    SceneInfoWindow& get_scene_info_window() { return m_scene_info_window; }
    MenuBar& get_menubar() { return m_menubar; }
    FileExplorer& get_file_explorer() { return m_file_explorer; }
  private:
    Gizmo m_gizmo;
    ItemSelectionWheel m_item_selection_wheel;
    SceneInfoWindow m_scene_info_window;
    MenuBar m_menubar;
    FileExplorer m_file_explorer;
    std::vector<UiComponent*> m_components;
  };
} // namespace fury
