#pragma once

#include "core/Common.hpp"
#include "UiComponent.hpp"
#include <glm/glm.hpp>

namespace fury
{
  class Scene;

  class SceneInfoWindow : public UiComponent
  {
  public:
    SceneInfoWindow() = default;
    void init(Scene* scene) override;
    void tick(float) override;
    Event<EntityId, bool> visible_normals_button_toggled;
    Event<EntityId, bool> visible_bbox_button_toggled;
    Event<EntityId, bool> light_visibility_button_toggled;
    Event<bool> fill_polygons_button_toggled;
    Event<bool> show_scene_bbox_button_toggled;
    Event<bool> use_msaa_button_toggled;
    Event<bool> frustum_culling_button_toggled;
    Event<bool> show_grid_button_toggled;
  private:
    void calculate_window_size(int w, int h);
    void render_entity_properties(EntityId entity);
    void render_fps_locks();
  private:
    glm::vec2 m_window_size;
    glm::vec2 m_window_pos;
  };
} // namespace fury
