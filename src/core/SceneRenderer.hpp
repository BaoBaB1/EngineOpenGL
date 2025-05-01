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
    std::vector<Object3D*>& get_selected_objects() { return m_selected_objects; }
    std::vector<std::unique_ptr<Object3D>>& get_drawables() { return m_drawables; }
    BoundingBox& get_bbox() { return m_bbox; }
    WindowGLFW* get_window() { return m_window; }
    Ui& get_ui() { return m_ui; }
    const DirectionalLight& get_directional_light() const { return m_directional_light; }
    void render();
    void save(const std::string& file) const;
    void load(const std::string& file);
    void clear();
    Event<Object3D*> on_new_object_added;
    Event<Object3D*> on_object_delete;
  private:
    void tick() override;
    //void create_scene();
    void select_object(Object3D* obj, bool click_from_menu_item);  // temporary function. remove when selection of multiple elements is supported
    void render_skybox();
    void handle_mouse_click(int button, int x, int y);
    void handle_window_size_change(int width, int height);
    void handle_keyboard_input(KeyboardHandler::InputKey key, KeyboardHandler::KeyState state);
    void change_polygon_mode(int new_mode);
    void handle_added_object(Object3D* obj);
    void handle_object_change(Object3D* obj, const ObjectChangeInfo& info);
    void calculate_scene_bbox();
    void update_shadow_map();
    void handle_file_explorer_opening();
    void handle_file_explorer_closing();
    void remove_object(Object3D* obj);
    friend class SceneInfo;
  private:
    // store pointers to make virtual methods work
    std::vector<std::unique_ptr<Object3D>> m_drawables;
    std::vector<Object3D*> m_selected_objects;
    std::vector<std::unique_ptr<RenderPass>> m_render_passes;
    std::unique_ptr<ShadowsPass> m_shadows_pass;
    std::unique_ptr<DebugPass> m_debug_pass;
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
