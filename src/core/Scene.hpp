#pragma once

#include "Camera.hpp"
#include "opengl/FrameBufferObject.hpp"
#include "input/InputSystem.hpp"
#include "CameraController.hpp"
#include "ui/Ui.hpp"
#include "ScreenQuad.hpp"
#include "ITickable.hpp"
#include "ge/Skybox.hpp"
#include "RenderPass.hpp"
#include "ge/BoundingBox.hpp"
#include "FPSLimiter.hpp"
#include "ge/ItemSelectionWheel.hpp"
#include "Light.hpp"
#include "Serialization.hpp"
#include "ge/Object3D.hpp"
#include "core/ObjectController.hpp"
#include "Singleton.hpp"
#include "RenderInfo.hpp"
#include <vector>
#include <memory>
#include <string>
#include <map>

namespace fury
{
  class Skybox;
  struct ObjectChangeInfo;

  class Scene : public ITickable, public Singleton<Scene>
  {
  public:
    FURY_REGISTER_BASE_CLASS(Scene)
    void tick(float dt) override;
    void init(WindowGLFW* window);
    ~Scene();
    const RenderInfo& get_render_info() const { return m_render_info; }
    Camera& get_camera() { return m_camera; }
    std::vector<Object3D*>& get_selected_objects() { return m_selected_objects; }
    std::vector<std::unique_ptr<Object3D>>& get_drawables() { return m_drawables; }
    BoundingBox& get_bbox() { return m_bbox; }
    WindowGLFW* get_window() { return m_window; }
    Ui& get_ui() { return m_ui; }
    std::vector<const Light*> get_active_lights() const;
    std::vector<Light>& get_lights() { return m_lights; }
    std::vector<Light*> get_lights(LightType type);
    void create_default_scene();
    void render();
    void save(const std::string& file) const;
    void load(const std::string& file);
    void clear();
    void set_fps_limit(uint32_t fps) { m_fps_limiter.set_limit(fps); }
    uint32_t get_fps_limit() const { return m_fps_limiter.get_limit(); }
    Event<Object3D*> on_new_object_added;
    Event<Object3D*> on_object_deleted;
    // These have to be complete types...
    FURY_DECLARE_SERIALIZABLE_FIELDS(
      FURY_SERIALIZABLE_FIELD(1, &Scene::m_camera),
      FURY_SERIALIZABLE_FIELD(2, &Scene::m_polygon_mode),
      FURY_SERIALIZABLE_FIELD(3, &Scene::m_drawables),
      FURY_SERIALIZABLE_FIELD(4, &Scene::m_lights),
      FURY_SERIALIZABLE_FIELD(5, &Scene::m_controllers)
    )
  private:
    Scene() = default;
    friend class Singleton<Scene>;
  private:
    void prepare_scene_for_rendering();
    void select_object(Object3D* obj, bool click_from_menu_item);  // temporary function. remove when selection of multiple elements is supported
    void render_skybox();
    void handle_mouse_click(InputCode, int x, int y);
    void handle_window_size_change(int width, int height);
    void handle_keyboard_button_click(InputCode input_code);
    void change_polygon_mode(int new_mode);
    void handle_object_change(const ObjectChangeInfo& info);
    void calculate_scene_bbox();
    void update_shadow_map();
    void handle_ui_component_opening();
    void handle_ui_component_closing();
    void handle_msaa_button_toggle(bool enabled);
    void remove_object(Object3D* obj);
    void cleanup();
    void setup_directional_light(Light* light);
    void create_default_lights();
    friend class SceneInfo;
  private:
    // store pointers to make virtual methods work
    std::vector<std::unique_ptr<Object3D>> m_drawables;
    std::vector<Object3D*> m_selected_objects;
    std::vector<std::unique_ptr<RenderPass>> m_render_passes;
    std::unique_ptr<ShadowsPass> m_shadows_pass;
    std::vector<Light> m_lights;
    std::vector<std::unique_ptr<ObjectController>> m_controllers;
    ScreenQuad m_screen_quad;
    ScreenQuad m_shadow_map_quad;
    bool m_show_shadow_map = false;
    bool m_MSAA_enabled = true;
    Skybox m_skybox;
    WindowGLFW* m_window;
    Ui m_ui;
    CameraController m_cam_controller;
    Camera m_camera;
    std::map<std::string, FrameBufferObject> m_fbos;
    GLint m_polygon_mode = GL_FILL;
    BoundingBox m_bbox;
    FPSLimiter m_fps_limiter;
    ItemSelectionWheel m_selection_wheel;
    RenderInfo m_render_info;
  };
}
