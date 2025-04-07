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
#include <vector>
#include <memory>
#include <map>
#include <unordered_set>

namespace fury
{
  class Object3D;
  class Skybox;

  class SceneRenderer : public ITickable
  {
  public:
    SceneRenderer(WindowGLFW* window);
    ~SceneRenderer();
    Camera& get_camera() { return m_camera; }
    std::unordered_set<int>& get_light_sources() { return m_light_sources; }
    std::vector<Object3D*>& get_selected_objects() { return m_selected_objects; }
    std::vector<std::unique_ptr<Object3D>>& get_drawables() { return m_drawables; }
    WindowGLFW* get_window() { return m_window; }
    Ui& get_ui() { return m_ui; }
    void render();
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
    friend class SceneInfo;
  private:
    // store pointers to make virtual methods work
    std::vector<std::unique_ptr<Object3D>> m_drawables;
    std::vector<Object3D*> m_selected_objects;
    std::vector<std::unique_ptr<RenderPass>> m_render_passes;
    std::unordered_set<int> m_light_sources;
    ScreenQuad m_screen_quad;
    Skybox m_skybox;
    WindowGLFW* m_window;
    Ui m_ui;
    CameraController m_cam_controller;
    Camera m_camera;
    std::map<std::string, FrameBufferObject> m_fbos;
    GLint m_polygon_mode = GL_FILL;
  };
}
