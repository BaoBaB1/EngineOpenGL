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

class SceneRenderer
{
public:
  static SceneRenderer& instance() { return OpenGLEngineUtils::Singleton<SceneRenderer>::instance(); }
  void render();
private:
  SceneRenderer();
  ~SceneRenderer();
  void handle_input();
  void create_scene();
  void select_object(int index, bool click_from_menu_item);  // temporary function. remove when selection of multiple elements is supported
  void new_frame_update();
  void render_scene();
  void render_picking_fbo();
  void render_selected_objects();
  void render_lines();
  void render_normals();
  void render_skybox(const Skybox& skybox);
  friend class MouseInputHandler;
  friend class CursorPositionHandler;
  friend class OpenGLEngineUtils::Singleton<SceneRenderer>;
  friend class Ui;
private:
  std::vector<std::unique_ptr<Object3D>> m_drawables;
  std::vector<int> m_selected_objects;
  std::unique_ptr<MainWindow> m_window;
  std::unique_ptr<GPUBuffers> m_gpu_buffers;
  std::unique_ptr<Ui> m_ui;
  Camera m_camera;
  std::map<std::string, FrameBufferObject> m_fbos;
  glm::mat4 m_projection_mat;
  GLint m_polygon_mode = GL_FILL;
};

struct ScreenQuad : IDrawable
{
  ScreenQuad(GLuint tex_id) : m_tex_id(tex_id) {}
  void render(GPUBuffers*, Shader& shader);
  bool has_surface() const override { return false; }
  std::string name() const override { return "ScreenQuad"; }
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
