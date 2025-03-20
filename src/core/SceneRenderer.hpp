#pragma once

#include "Shader.hpp"
#include "Camera.hpp"
#include "FrameBufferObject.hpp"
#include "GPUBuffers.hpp"
#include "KeyboardHandler.hpp"
#include "CameraController.hpp"
#include "Ui.hpp"
#include <vector>
#include <memory>
#include <map>

class Object3D;
class Skybox;

class SceneRenderer
{
public:
  SceneRenderer(WindowGLFW* window);
  ~SceneRenderer();
  Camera& get_camera() { return m_camera; }
  void render();
private:
  void create_scene();
  void select_object(Object3D* obj, bool click_from_menu_item);  // temporary function. remove when selection of multiple elements is supported
  void on_new_frame();
  void render_scene();
  void render_selected_objects();
  void render_lines();
  void render_normals();
  void render_skybox(const Skybox& skybox);
  void handle_mouse_click(int button, int x, int y);
  void handle_window_size_change(int width, int height);
  void handle_keyboard_input(KeyboardHandler::InputKey key, KeyboardHandler::KeyState state);
  friend class Ui;
private:
  std::vector<std::unique_ptr<Object3D>> m_drawables;
  std::vector<Object3D*> m_selected_objects;
  WindowGLFW* m_window;
  GPUBuffers m_gpu_buffers;
  Ui m_ui;
  CameraController m_cam_controller;
  Camera m_camera;
  std::map<std::string, FrameBufferObject> m_fbos;
  GLint m_polygon_mode = GL_FILL;
};

struct ScreenQuad
{
  ScreenQuad(GLuint tex_id) : m_tex_id(tex_id)
  {
    vao.bind();
    vbo.bind();
    vbo.set_data(quadVertices, sizeof(quadVertices));
    vao.link_attrib(0, 2, GL_FLOAT, sizeof(float) * 4, nullptr);
    vao.link_attrib(1, 2, GL_FLOAT, sizeof(float) * 4, (void*)(sizeof(float) * 2));
    vao.unbind();
    vbo.unbind();
  }
  void render();
  VertexArrayObject vao;
  VertexBufferObject vbo;
  GLuint m_tex_id;

  static constexpr float quadVertices[] =
  { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
    // positions   // texCoords
    -1.0f,  1.0f,  0.0f, 1.0f,
    -1.0f, -1.0f,  0.0f, 0.0f,
     1.0f, -1.0f,  1.0f, 0.0f,

    -1.0f,  1.0f,  0.0f, 1.0f,
     1.0f, -1.0f,  1.0f, 0.0f,
     1.0f,  1.0f,  1.0f, 1.0f
  };
};
