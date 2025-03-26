#pragma once

#include "Shader.hpp"
#include "Camera.hpp"
#include "FrameBufferObject.hpp"
#include "KeyboardHandler.hpp"
#include "CameraController.hpp"
#include "Ui.hpp"
#include "ScreenQuad.hpp"
#include "ITickable.hpp"
#include "ge/Skybox.hpp"
#include "PipelineBufferManager.hpp"
#include <vector>
#include <memory>
#include <map>
#include <unordered_set>

class Object3D;
class Skybox;

class SceneRenderer : public ITickable
{
public:
  SceneRenderer(WindowGLFW* window);
  ~SceneRenderer();
  Camera& get_camera() { return m_camera; }
  void render();
private:
  void tick() override;
  void create_scene();
  void select_object(Object3D* obj, bool click_from_menu_item);  // temporary function. remove when selection of multiple elements is supported
  void render_scene();
  void render_selected_objects();
  void render_lines();
  void render_normals();
  void render_skybox();
  void handle_mouse_click(int button, int x, int y);
  void handle_window_size_change(int width, int height);
  void handle_keyboard_input(KeyboardHandler::InputKey key, KeyboardHandler::KeyState state);
  void handle_visible_normals_click(Object3D* obj, bool is_selected);
  void prepare_pipeline_buffers();
  friend class Ui;
private:
  std::vector<std::unique_ptr<Object3D>> m_drawables;
  std::vector<Object3D*> m_selected_objects;
  std::unordered_set<Object3D*> m_objects_with_visible_normals;
  bool m_need_normal_data_update = true; // update on first run
  ScreenQuad m_screen_quad;
  Skybox m_skybox;
  WindowGLFW* m_window;
  Ui m_ui;
  CameraController m_cam_controller;
  Camera m_camera;
  std::map<std::string, FrameBufferObject> m_fbos;
  GLint m_polygon_mode = GL_FILL;
};
