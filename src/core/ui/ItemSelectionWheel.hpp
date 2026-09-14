#pragma once

#include "core/Common.hpp"
#include "core/AssetHandle.hpp"
#include "core/ui/UiComponent.hpp"
#include <glm/glm.hpp>
#include <vector>

namespace fury
{
  class Texture2D;
  class Scene;

  struct SelectionWheelConfig
  {
    int items_count = 0;
    int inner_circle_radius_px = 0;
    int outer_circle_radius_px = 0;
    float slots_spacing_deg = 15.f;
  };

  struct SelectionWheelSlot
  {
    std::vector<glm::vec2> arc_segments;
    glm::vec2 arcs_closing_lines[4] = {};
    glm::vec2 angles;
    glm::vec2 center;
    int idx = -1;
    float icon_size = 0.f;
    AssetHandle<Texture2D> icon;
  };

  class ItemSelectionWheel : public UiComponent
  {
  public:
    ItemSelectionWheel();
    void set_config(const SelectionWheelConfig& config);
    std::vector<SelectionWheelSlot>& get_slots() { return m_slots; }
    void select(int slot) { m_selected_slot = slot; }
    void tick(float dt) override {};
    SelectionWheelSlot* get_slot(int idx) { return &m_slots[idx]; }
    SelectionWheelSlot* get_selected_slot() { return (m_selected_slot == -1) ? nullptr : &m_slots[m_selected_slot]; }
    const SelectionWheelConfig& get_config() const { return m_config; }
  private:
    void handle_cursor_position_change(double newx, double newy, double oldx, double oldy);
  private:
    std::vector<SelectionWheelSlot> m_slots;
    SelectionWheelConfig m_config;
    int m_selected_slot = -1;
  };
} // namespace fury
