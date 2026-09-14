#pragma once

#include "Camera.hpp"
#include "opengl/FrameBufferObject.hpp"
#include "opengl/UniformBuffer.hpp"
#include "opengl/SSBO.hpp"
#include "input/InputSystem.hpp"
#include "CameraController.hpp"
#include "ui/Ui.hpp"
#include "ScreenQuad.hpp"
#include "ITickable.hpp"
#include "Skybox.hpp"
#include "BoundingBox.hpp"
#include "FPSLimiter.hpp"
#include "Mesh.hpp"
#include "Serialization.hpp"
#include "Singleton.hpp"
#include "FrameInfo.hpp"
#include "RenderPass.hpp"
#include "ecs/ComponentManager.hpp"
#include "ecs/EntityManager.hpp"
#include <vector>
#include <memory>
#include <string>
#include <map>
#include <mutex>
#include <tuple>

namespace fury
{
  class SceneNode;

  namespace ecs
  {
    struct DrawableComponent;
    struct TransformComponent;
  } // namespace ecs

  struct SceneOptions
  {
    // < 0 - no cap, 0 - vsync, > 0 set cap
    int fps_cap = 0;
    bool fill_polygons = true;
    bool show_scene_bbox = false;
    bool use_msaa = true;
    bool is_vsync_enabled = true;
    bool show_grid = false;
    bool is_frustum_culling_enabled = true;
    bool show_shadow_map = false;
  };

  class Scene : public ITickable
  {
  public:
    Scene();
    ~Scene();
    void tick(float dt) override;
    void init(WindowGLFW* window);
    std::tuple<EntityId, ecs::DrawableComponent&, ecs::TransformComponent&>
    add_entity(const std::vector<MeshNode>& mesh_nodes, bool has_surface);
    void remove_entity(EntityId entity);
    const FrameInfo& get_frame_info() const { return m_frame_info; }
    const Camera& get_camera() const { return m_camera; }
    Camera& get_camera() { return m_camera; }
    EntityId get_selected_entity() const { return m_selected_entity; }
    const std::set<EntityId>& get_entities_with_visible_normals() const { return m_entities_with_visible_normals; }
    const std::set<EntityId>& get_entities_with_visible_bboxes() const { return m_entities_with_visible_bboxes; }
    const std::unordered_map<EntityId, uint32_t>& get_entity_transform_ssbo_idx_map() const
    {
      return m_entity_transform_ssbo_idx_map;
    }
    const std::unordered_map<EntityId, uint32_t>& get_entity_light_ssbo_idx_map() const
    {
      return m_entity_light_ssbo_idx_map;
    }
    SSBO& get_transforms_ssbo() { return m_transforms_ssbo; }
    SSBO& get_lights_ssbo() { return m_lights_ssbo; }
    uint32_t get_fps() const { return m_fps; }
    SceneOptions& get_options() { return m_scene_options; }
    const SceneOptions& get_options() const { return m_scene_options; }
    ecs::EntityManager& get_entity_manager() { return m_entity_manager; }
    ecs::ComponentManager& get_component_manager() { return m_component_manager; }
    BoundingBox& get_bbox() { return m_bbox; }
    WindowGLFW* get_window() { return m_window; }
    Ui& get_ui() { return m_ui; }
    void create_default_scene();
    void render();
    void save(const std::filesystem::path& file) const;
    void load(const std::filesystem::path& file);
    void clear();
    void clear_geometry();
    void set_fps_limit(uint32_t fps) { m_fps_limiter.set_limit(fps); }
    void enqueue_job_from_async_thread(std::move_only_function<void()> job);
    uint32_t get_fps_limit() const { return m_fps_limiter.get_limit(); }
    //  These have to be complete types...
    // clang-format off
    FURY_DECLARE_SERIALIZABLE_FIELDS(
      FURY_SERIALIZABLE_FIELD(1, &Scene::m_camera),
      FURY_SERIALIZABLE_FIELD(2, &Scene::m_scene_options),
      FURY_SERIALIZABLE_FIELD(3, &Scene::m_entities_with_visible_bboxes),
      FURY_SERIALIZABLE_FIELD(4, &Scene::m_entities_with_visible_normals)
    )
    // clang-format on
  private:
    void init_ui_components();
    void prepare_scene_for_rendering();
    void process_async_results();
    void select_object(EntityId entity, bool click_from_menu_item);
    void render_skybox();
    void handle_mouse_click(InputCode, int x, int y);
    void handle_window_size_change(int width, int height);
    void handle_keyboard_button_click(InputCode input_code);
    void update_transforms_ssbo();
    void update_transform_ssbo_entry(EntityId entity);
    void update_lights_ssbo();
    void update_light_ssbo_entry(EntityId entity);
    void calculate_scene_bbox();
    void update_shadow_map();
    void handle_ui_component_opening();
    void handle_ui_component_closing();
    void on_entity_normals_visibility_toggle(EntityId entity, bool visible_normals);
    void on_entity_bbox_visibility_toggle(EntityId entity, bool visible_bbox);
    void create_default_lights();
    void update_geometry_pipeline();
    friend class SceneInfoWindow;
  private:
    Camera m_camera;
    Skybox m_skybox;
    std::mutex m_async_tasks_mutex;
    ecs::ComponentManager m_component_manager;
    ecs::EntityManager m_entity_manager;
    ScreenQuad m_screen_quad;
    ScreenQuad m_shadow_map_quad;
    std::vector<std::move_only_function<void()>> m_async_tasks;
    std::vector<RenderPass*> m_render_passes;
    std::unordered_map<EntityId, uint32_t> m_entity_transform_ssbo_idx_map;
    std::unordered_map<EntityId, uint32_t> m_entity_light_ssbo_idx_map;
    EntityId m_selected_entity = g_invalid_entity_id;
    std::set<EntityId> m_entities_with_visible_bboxes;
    std::set<EntityId> m_entities_with_visible_normals;
    std::map<std::string, FrameBufferObject> m_fbos;
    GeometryPass m_geometry_pass;
    ShadowsPass m_shadows_pass;
    InfiniteGridPass m_grid_pass;
    DebugPass m_debug_pass;
    SelectionWheelPass m_selection_wheel_pass;
    UniformBuffer m_camera_data_ubo;
    SSBO m_transforms_ssbo;
    SSBO m_lights_ssbo;
    Ui m_ui;
    BoundingBox m_bbox;
    FPSLimiter m_fps_limiter;
    CameraController m_cam_controller;
    FrameInfo m_frame_info;
    SceneOptions m_scene_options;
    WindowGLFW* m_window = nullptr;
    uint32_t m_fps = 0;
    bool m_update_geometry_pipeline = false;
  };
} // namespace fury
