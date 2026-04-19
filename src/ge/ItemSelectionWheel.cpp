#include "ItemSelectionWheel.hpp"
#include "utils/Constants.hpp"
#include "core/Logger.hpp"
#include "core/opengl/Texture2D.hpp"
#include "core/WindowGLFW.hpp"
#include "core/input/InputSystem.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace fury
{
  void ItemSelectionWheel::init(int window_width, int window_height, const SelectionWheelConfig& config)
  {
    m_window_size[0] = window_width;
    m_window_size[1] = window_height;
    m_config = config;
    InputSystem::instance().on_cursor_moved +=
        new InstanceListener(this, &ItemSelectionWheel::handle_cursor_position_change);

    constexpr int segments = 15;
    const float item_spacing = glm::radians(config.slots_spacing_deg);
    const int radiuses[2] = { config.inner_circle_radius_px, config.outer_circle_radius_px };
    const float item_angle = (2.f * constants::PI_F) / config.items_count;
    const float angle_step = item_angle / segments;

    m_slots.clear();
    m_slots.resize(config.items_count);
    for (int i = 0; i < config.items_count; i++)
    {
      m_slots[i].arcs_data.reserve(segments);
    }

    for (int item = 0; item < config.items_count; item++)
    {
      SelectionWheelSlot& slot = m_slots[item];
      const float start_angle = item * item_angle + item_spacing;
      slot.angles = { start_angle, start_angle + item_angle };
      // 2 circles
      for (int j = 0; j < 2; j++)
      {
        const int r = radiuses[j];
        for (int seg = 0; seg <= segments; seg++)
        {
          float theta = start_angle + seg * angle_step;
          const float x = r * glm::cos(theta);
          const float y = r * glm::sin(theta);
          slot.arcs_data.emplace_back(glm::vec2 { x, y });
        }
      }
      slot.center = (slot.arcs_data[segments / 2 + 1] + slot.arcs_data[segments + segments / 2 + 1]) / 2.f;
      slot.icon_size = (radiuses[1] - radiuses[0]) / 3.f;
      slot.arcs_closing_lines_data[0] = slot.arcs_data[0];
      slot.arcs_closing_lines_data[1] = slot.arcs_data[segments + 1];
      slot.arcs_closing_lines_data[2] = slot.arcs_data[segments];
      slot.arcs_closing_lines_data[3] = slot.arcs_data[2 * segments + 1];
      slot.idx = item;
    }
  }

  void ItemSelectionWheel::handle_cursor_position_change(double newx, double newy, double oldx, double oldy)
  {
    if (!m_is_visible)
      return;
    const glm::vec2 pos = { newx, newy };
    const glm::vec2 center = { m_window_size[0] / 2.f, m_window_size[1] / 2.f };
    if (pos == center)
      return;
    const glm::vec2 dir = glm::normalize(pos - center);
    float angle = std::atan2(-dir.y, dir.x);
    float offset = glm::radians(m_config.slots_spacing_deg);
    // angle += glm::radians(m_config.slots_spacing_deg);
    if (angle < 0)
      angle += constants::PI2_F;

    int slot = -1;
    // last slot's end is always > 2Pi because of slot spacings (if spacing > 0)
    if (angle < offset)
    {
      slot = m_slots.size() - 1;
    }
    else
    {
      for (int i = 0; i < m_slots.size(); i++)
      {
        if (angle >= m_slots[i].angles.x && angle <= m_slots[i].angles.y)
        {
          slot = i;
        }
      }
    }
    select(slot);
  }
} // namespace fury
