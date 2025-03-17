#pragma once

#include "Shader.hpp"
#include "Camera.hpp"
#include "FrameBufferObject.hpp"
#include "ge/IDrawable.hpp"
#include "utils/Singleton.hpp"
#include "GPUBuffers.hpp"
#include <vector>
#include <memory>
#include <map>

class MouseInputHandler;
class CursorPositionHandler;
class Ui;
class Object3D;
class MainWindow;
class Skybox;
struct GLFWwindow;

class SceneRenderer
{
public:
  static SceneRenderer& instance() { return OpenGLEngineUtils::Singleton<SceneRenderer>::instance(); }
  Camera& get_camera() { return m_camera; }
  void render();
private:
  SceneRenderer();
  ~SceneRenderer();
  void handle_input();
  void create_scene();
  void select_object(Object3D* obj, bool click_from_menu_item);  // temporary function. remove when selection of multiple elements is supported
  void new_frame_update();
  void render_scene();
  void render_selected_objects();
  void render_lines();
  void render_normals();
  void render_skybox(const Skybox& skybox);
  void handle_mouse_click(int button, int x, int y);
  void handle_window_size_change(int width, int height);
  friend class OpenGLEngineUtils::Singleton<SceneRenderer>;
  friend class Ui;
private:
  std::vector<std::unique_ptr<Object3D>> m_drawables;
  std::vector<Object3D*> m_selected_objects;
  std::unique_ptr<MainWindow> m_window;
  std::unique_ptr<GPUBuffers> m_gpu_buffers;
  // TODO: move to some other place
  std::unique_ptr<VertexBufferObject> m_skybox_vbo;
  std::unique_ptr<VertexArrayObject> m_skybox_vao;
  std::unique_ptr<Ui> m_ui;
  Camera m_camera;
  std::map<std::string, FrameBufferObject> m_fbos;
  GLint m_polygon_mode = GL_FILL;
};

struct ScreenQuad : IDrawable
{
  ScreenQuad(GLuint tex_id) : m_tex_id(tex_id)
  {
    vao->bind();
    vbo->bind();
    vbo->set_data(quadVertices, sizeof(quadVertices));
    vao->link_attrib(0, 2, GL_FLOAT, sizeof(float) * 4, nullptr);
    vao->link_attrib(1, 2, GL_FLOAT, sizeof(float) * 4, (void*)(sizeof(float) * 2));
    vao->unbind();
    vbo->unbind();
  }
  void render();
  bool has_surface() const override { return false; }
  std::string name() const override { return "ScreenQuad"; }
  std::unique_ptr<VertexArrayObject> vao = std::make_unique<VertexArrayObject>();
  std::unique_ptr<VertexBufferObject> vbo = std::make_unique<VertexBufferObject>();
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
