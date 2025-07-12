#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace fury
{
  class WindowGLFW;
  class Texture2D;

  struct SelectionWheelConfig
  {
    int items_count = 0;
    int inner_circle_radius_px = 0;
    int outer_circle_radius_px = 0;
    float slots_spacing_deg = 15.f;
  };

  struct SelectionWheelSlot
  {
    std::vector<glm::vec2> arcs_data;
    glm::vec2 arcs_closing_lines_data[4] = {};
    glm::vec2 angles;
    glm::vec2 center;
    int idx = -1;
    float icon_size = 0.f;
    Texture2D* icon = nullptr;
  };

  class ItemSelectionWheel
  {
  public:
    ItemSelectionWheel() = default;
    void init(WindowGLFW* window, const SelectionWheelConfig& config);
    std::vector<SelectionWheelSlot>& get_slots() { return m_slots; }
    bool is_visible() const { return m_is_visible; }
    void set_is_visible(bool v) { m_is_visible = v; }
    void select(int slot) { m_selected_slot = slot; }
    SelectionWheelSlot* get_slot(int idx) { return &m_slots[idx]; }
    SelectionWheelSlot* get_selected_slot() { return (m_selected_slot == -1) ? nullptr : &m_slots[m_selected_slot]; }
    const SelectionWheelConfig& get_config() const { return m_config; }
  private:
    void handle_cursor_position_change(double newx, double newy, double oldx, double oldy);
  private:
    std::vector<SelectionWheelSlot> m_slots;
    SelectionWheelConfig m_config;
    bool m_is_visible = false;
    int m_selected_slot = -1;
    WindowGLFW* m_window = nullptr;
  };
}
