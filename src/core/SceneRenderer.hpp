#pragma once

#include "Camera.hpp"
#include "opengl/FrameBufferObject.hpp"
#include "input/KeyboardHandler.hpp"
#include "CameraController.hpp"
#include "ui/Ui.hpp"
#include "ScreenQuad.hpp"
#include "ITickable.hpp"
#include "ge/Skybox.hpp"
#include "RenderPass.hpp"
#include "ge/BoundingBox.hpp"
#include "ObjectChangeInfo.hpp"
#include <vector>
#include <memory>
#include <string>
#include <map>
#include <unordered_set>

namespace fury
{
  class Object3D;
  class Skybox;

  struct DirectionalLight
  {
    glm::mat4 proj_matrix;;
    glm::mat4 view_matrix;
    glm::vec3 direction;
  };

  class SceneRenderer : public ITickable
  {
  public:
    SceneRenderer(WindowGLFW* window);
    ~SceneRenderer();
    Camera& get_camera() { return m_camera; }
    std::unordered_set<int>& get_light_sources() { return m_light_sources; }
    std::vector<Object3D*>& get_selected_objects() { return m_selected_objects; }
    std::vector<std::unique_ptr<Object3D>>& get_drawables() { return m_drawables; }
    BoundingBox& get_bbox() { return m_bbox; }
    WindowGLFW* get_window() { return m_window; }
    Ui& get_ui() { return m_ui; }
    const DirectionalLight& get_directional_light() const { return m_directional_light; }
    void render();
    void save(const std::string& file) const;
    void load(const std::string& file);
    Event<Object3D*> on_new_object_added;
  private:
    void tick() override;
    void create_scene();
    void select_object(Object3D* obj, bool click_from_menu_item);  // temporary function. remove when selection of multiple elements is supported
    void render_skybox();
    void handle_mouse_click(int button, int x, int y);
    void handle_window_size_change(int width, int height);
    void handle_keyboard_input(KeyboardHandler::InputKey key, KeyboardHandler::KeyState state);
    void change_polygon_mode(int new_mode);
    void handle_new_added_object(Object3D* obj);
    void handle_object_change(Object3D* obj, const ObjectChangeInfo& info);
    void calculate_scene_bbox();
    void update_shadow_map();
    friend class SceneInfo;
  private:
    // store pointers to make virtual methods work
    std::vector<std::unique_ptr<Object3D>> m_drawables;
    std::vector<Object3D*> m_selected_objects;
    std::vector<std::unique_ptr<RenderPass>> m_render_passes;
    std::unique_ptr<ShadowsPass> m_shadows_pass;
    std::unordered_set<int> m_light_sources;
    ScreenQuad m_screen_quad;
    ScreenQuad m_shadow_map_quad;
    bool m_show_shadow_map = false;
    Skybox m_skybox;
    WindowGLFW* m_window;
    Ui m_ui;
    CameraController m_cam_controller;
    Camera m_camera;
    std::map<std::string, FrameBufferObject> m_fbos;
    GLint m_polygon_mode = GL_FILL;
    BoundingBox m_bbox;
    DirectionalLight m_directional_light;
  };
}
