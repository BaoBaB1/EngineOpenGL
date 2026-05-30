#pragma once

#include "Camera.hpp"
#include "opengl/FrameBufferObject.hpp"
#include "opengl/UniformBuffer.hpp"
#include "input/InputSystem.hpp"
#include "CameraController.hpp"
#include "ui/Ui.hpp"
#include "ScreenQuad.hpp"
#include "ITickable.hpp"
#include "ge/Skybox.hpp"
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
#include <future>
#include <mutex>

namespace fury
{
  class Skybox;
  struct ObjectChangeInfo;
  class ObjectController;
  class RenderPass;
  class ShadowsPass;

  enum class DeferredCommandContext
  {
    ADD_OBJECT
  };

  class Scene : public ITickable, public Singleton<Scene>
  {
  public:
    FURY_REGISTER_BASE_CLASS(Scene)
    ~Scene();
    template<typename T>
    void add_async_task(std::future<T> future, DeferredCommandContext ctx)
    {
      std::lock_guard lg(m_async_tasks_vec_mutex);
      if (ctx == DeferredCommandContext::ADD_OBJECT)
      {
        auto wrapped_fut = std::make_shared<decltype(future)>(std::move(future));
        auto sp = std::make_shared<std::function<bool()>>(
            [=]() mutable
            {
              // Return true if future is ready and false otherwise, so that if future is not ready yet
              // main thread won't wait and will proceed rendering current frame and recheck condition on the next one
              if (wrapped_fut->wait_for(std::chrono::seconds(0)) == std::future_status::ready)
              {
                std::unique_ptr<Object3D> obj = wrapped_fut->get();
                for (Mesh& m : obj->get_meshes())
                {
                  for (int i = 0; i < static_cast<int>(TextureType::LAST); i++)
                  {
                    if (auto tex = m.get_texture(static_cast<TextureType>(i)))
                    {
                      if (!tex->is_created())
                      {
                        // At this point texture data should be read (done in other thread)
                        // but OpenGL texture is not created via glTex* calls yet.
                        assert(tex->is_initialized());
                        tex->create();
                      }
                    }
                  }
                }
                add_object(std::move(obj));
                return true;
              }
              return false;
            });
        m_async_tasks.emplace_back([sp]() { return (*sp)(); });
      }
    }
    void tick(float dt) override;
    void init(WindowGLFW* window);
    void add_object(std::unique_ptr<Object3D> object);
    void remove_object(Object3D* obj);
    void remove_object(uint32_t id);
    const RenderInfo& get_render_info() const { return m_render_info; }
    const Camera& get_camera() const { return m_camera; }
    const std::vector<Object3D*>& get_selected_objects() const { return m_selected_objects; }
    const std::vector<std::unique_ptr<Object3D>>& get_drawables() { return m_drawables; }
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
    Event<Object3D*> on_object_about_be_deleted;
    // These have to be complete types...
    // clang-format off
    FURY_DECLARE_SERIALIZABLE_FIELDS(
      FURY_SERIALIZABLE_FIELD(1, &Scene::m_camera),
      FURY_SERIALIZABLE_FIELD(2, &Scene::m_polygon_mode),
      FURY_SERIALIZABLE_FIELD(3, &Scene::m_drawables),
      FURY_SERIALIZABLE_FIELD(4, &Scene::m_lights),
      FURY_SERIALIZABLE_FIELD(5, &Scene::m_controllers)
    )
    // clang-format on
  private:
    Scene();
    friend class Singleton<Scene>;
  private:
    void prepare_scene_for_rendering();
    void process_async_results();
    void select_object(
        Object3D* obj,
        bool click_from_menu_item); // temporary function. remove when selection of multiple elements is supported
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
    void cleanup();
    void setup_directional_light(Light* light);
    void create_default_lights();
    friend class SceneInfo;
  private:
    Camera m_camera;
    Skybox m_skybox;
    std::mutex m_async_tasks_vec_mutex;
    ScreenQuad m_screen_quad;
    ScreenQuad m_shadow_map_quad;
    ItemSelectionWheel m_selection_wheel;
    // store pointers to make virtual methods work
    std::vector<std::unique_ptr<Object3D>> m_drawables;
    std::vector<Object3D*> m_selected_objects;
    std::vector<std::unique_ptr<RenderPass>> m_render_passes;
    std::unique_ptr<ShadowsPass> m_shadows_pass;
    std::vector<Light> m_lights;
    std::vector<std::unique_ptr<ObjectController>> m_controllers;
    std::vector<std::function<bool()>> m_async_tasks;
    std::map<std::string, FrameBufferObject> m_fbos;
    UniformBuffer m_camera_data_ubo;
    Ui m_ui;
    BoundingBox m_bbox;
    FPSLimiter m_fps_limiter;
    CameraController m_cam_controller;
    RenderInfo m_render_info;
    WindowGLFW* m_window = nullptr;
    GLint m_polygon_mode = GL_FILL;
    bool m_show_shadow_map = false;
    bool m_MSAA_enabled = true;
  };
} // namespace fury
