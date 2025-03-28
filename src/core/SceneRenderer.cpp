#include "SceneRenderer.hpp"
#include "WindowGLFW.hpp"
#include "Shader.hpp"
#include "VertexArrayObject.hpp"
#include "VertexBufferObject.hpp"
#include "ElementBufferObject.hpp"
#include "Debug.hpp"
#include "Camera.hpp"
#include "KeyboardHandler.hpp"
#include "CursorPositionHandler.hpp"
#include "MouseInputHandler.hpp"
#include "ShaderStorage.hpp"
#include "BindGuard.hpp"
#include "ge/Cube.hpp"
#include "ge/Icosahedron.hpp"
#include "ge/Polyline.hpp"
#include "ge/Pyramid.hpp"
#include "ge/BezierCurve.hpp"
#include "ge/Skybox.hpp"

#include "imgui.h"
#include "imgui_internal.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>

#include <cassert>

#define DEBUG_RAY 0

namespace
{
  void setup_opengl();
  constexpr std::array<std::string_view, 6> skybox_faces =
  {
    ".\\.\\src\\textures\\skybox\\right.jpg",
    ".\\.\\src\\textures\\skybox\\left.jpg",
    ".\\.\\src\\textures\\skybox\\top.jpg",
    ".\\.\\src\\textures\\skybox\\bottom.jpg",
    ".\\.\\src\\textures\\skybox\\front.jpg",
    ".\\.\\src\\textures\\skybox\\back.jpg"
  };
  constexpr std::string_view PIPELINE_BUFFERS_MAIN_RECORD_NAME = "MAIN";
  constexpr std::string_view PIPELINE_BUFFERS_NORMALS_RECORD_NAME = "NORMALS";
  void set_default_vertex_attributes(VertexArrayObject& vao)
  {
    vao.link_attrib(0, 3, GL_FLOAT, sizeof(Vertex), nullptr);                        // position
    vao.link_attrib(1, 3, GL_FLOAT, sizeof(Vertex), (void*)(sizeof(GLfloat) * 3));   // normal
    vao.link_attrib(2, 4, GL_FLOAT, sizeof(Vertex), (void*)(sizeof(GLfloat) * 6));   // color
    vao.link_attrib(3, 2, GL_FLOAT, sizeof(Vertex), (void*)(sizeof(GLfloat) * 10));  // texture
  }
}

SceneRenderer::SceneRenderer(WindowGLFW* window) : m_window(window)
{
  const int w = window->width();
  const int h = window->height();
  m_ui.init(this, m_window);
  m_ui.on_visible_normals_button_pressed += new InstanceListener(this, &::SceneRenderer::handle_visible_normals_click);
  m_camera.set_screen_size({ w, h });
  m_camera.set_position(glm::vec3(-4.f, 2.f, 3.f));
  m_camera.look_at(glm::vec3(2.f, 0.5f, 0.5f));

  CursorPositionHandler* cursor_pos_handler = static_cast<CursorPositionHandler*>(m_window->get_input_handler(UserInputHandler::CURSOR_POSITION));
  MouseInputHandler* mouse_input_handler = static_cast<MouseInputHandler*>(m_window->get_input_handler(UserInputHandler::MOUSE_INPUT));
  KeyboardHandler* keyboard_input_handler = static_cast<KeyboardHandler*>(m_window->get_input_handler(UserInputHandler::KEYBOARD));
  mouse_input_handler->on_button_click += new InstanceListener(this, &::SceneRenderer::handle_mouse_click);
  keyboard_input_handler->on_key_state_change += new InstanceListener(this, &::SceneRenderer::handle_keyboard_input);
  m_window->on_window_size_change += new InstanceListener(this, &::SceneRenderer::handle_window_size_change);
  m_cam_controller.init(&m_camera, keyboard_input_handler, cursor_pos_handler);
  ShaderStorage::init();

  auto& main_scene_fbo = m_fbos["main"];
  main_scene_fbo.bind();
  main_scene_fbo.attach_texture(w, h, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE);
  main_scene_fbo.attach_renderbuffer(w, h, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL_ATTACHMENT);
  main_scene_fbo.unbind();

  m_skybox = Cubemap(skybox_faces);
  m_screen_quad.set_texture_id(main_scene_fbo.texture()->id());

  {
    PipelineBuffersRecord& main_record = PipelineBuffersManager::instance().get(PIPELINE_BUFFERS_MAIN_RECORD_NAME.data());
    UniformBuffer& ubo = main_record.ubos.emplace_back();
    ubo.resize(sizeof(glm::mat4) * 2);
    ubo.set_binding_point(1);
    BindChainFIFO bind_chain({ &main_record.vao, &main_record.vbo });
    ::set_default_vertex_attributes(main_record.vao);
  }

  {
    PipelineBuffersRecord& normals_record = PipelineBuffersManager::instance().get(PIPELINE_BUFFERS_NORMALS_RECORD_NAME.data());
    SSBO& ssbo = normals_record.ssbos.emplace_back();
    ssbo.set_binding_point(2);
    BindChainFIFO bind_chain({ &normals_record.vao, &normals_record.vbo });
    ::set_default_vertex_attributes(normals_record.vao);
  }
}

SceneRenderer::~SceneRenderer()
{
}

void SceneRenderer::render()
{
  GLFWwindow* gl_window = m_window->gl_window();
  ::setup_opengl();
  create_scene();

  for (const auto& [name, fbo] : m_fbos)
  {
    fbo.bind();
    assert(fbo.is_complete());
    fbo.unbind();
  }
  const auto& main_fbo = m_fbos.at("main");

  while (!glfwWindowShouldClose(gl_window))
  {
    glfwPollEvents();
    tick();
    glPolygonMode(GL_FRONT_AND_BACK, m_polygon_mode);
    
    // render to a custom framebuffer
    main_fbo.bind();
    glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    // render scene before gui to make sure that imgui window always will be on top of drawn entities
    render_skybox();
    render_scene();
    if (!m_selected_objects.empty())
    {
      render_selected_objects();
    }
    render_normals();
    render_lines();
    m_ui.render();
    main_fbo.unbind();

    m_screen_quad.tick();
    glfwSwapBuffers(gl_window);
  }
}

void SceneRenderer::render_scene()
{
  assert(m_light_sources.size() == 1);
  Object3D* light_source = *m_light_sources.begin();

  Shader* shader = &ShaderStorage::get(ShaderStorage::ShaderType::MAIN);
  shader->bind();
  shader->set_vec3("viewPos", m_camera.position());
  shader->set_int("defaultTexture", 0);
  shader->set_int("ambientTex", 1);
  shader->set_int("diffuseTex", 2);
  shader->set_int("specularTex", 3);
  // center in world space
  shader->set_vec3("lightPos", light_source->center() + glm::vec3(light_source->m_model_mat[3]));
  shader->set_vec3("lightColor", glm::vec3(1.f));

  PipelineBuffersRecord& rec = PipelineBuffersManager::instance().get(PIPELINE_BUFFERS_MAIN_RECORD_NAME.data());
  BindChainFIFO bind_chain({ &rec.vao, &rec.vbo, &rec.ebo });
  for (const auto& pobj : m_drawables)
  {
    shader->set_matrix4f("modelMatrix", pobj->model_matrix());
    shader->set_bool("applyShading", pobj->m_shading_mode != Object3D::ShadingMode::NO_SHADING && !pobj->is_light_source());

    if (pobj->is_selected() && pobj->has_surface())
    {
      // enable writing to the stencil buffer and disable it in the end of current iteration
      glStencilFunc(GL_ALWAYS, 1, 0xFF);
      glStencilMask(0xFF);
    }

    const auto& render_config = pobj->get_render_config();
    const size_t mesh_count = pobj->mesh_count();
    for (size_t i = 0; i < mesh_count; i++)
    {
      const Mesh& mesh = pobj->get_mesh(i);

      // bind textures
      if (auto tex = mesh.get_texture(TextureType::GENERIC))
      {
        shader->set_bool("hasDefaultTexture", true);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex->id());
      }
      else
      {
        shader->set_bool("hasDefaultTexture", false);
      }
      if (auto tex = mesh.get_texture(TextureType::AMBIENT))
      {
        shader->set_bool("hasAmbientTex", true);
        glActiveTexture(GL_TEXTURE0 + 1);
        glBindTexture(GL_TEXTURE_2D, tex->id());
      }
      else
      {
        shader->set_bool("hasAmbientTex", false);
      }
      if (auto tex = mesh.get_texture(TextureType::DIFFUSE))
      {
        shader->set_bool("hasDiffuseTex", true);
        glActiveTexture(GL_TEXTURE0 + 2);
        glBindTexture(GL_TEXTURE_2D, tex->id());
      }
      else
      {
        shader->set_bool("hasDiffuseTex", false);
      }
      if (auto tex = mesh.get_texture(TextureType::SPECULAR))
      {
        shader->set_bool("hasSpecularTex", true);
        glActiveTexture(GL_TEXTURE0 + 3);
        glBindTexture(GL_TEXTURE_2D, tex->id());
      }
      else
      {
        shader->set_bool("hasSpecularTex", false);
      }

      auto& vbo = rec.vbo;
      const std::vector<Vertex>& vertices = mesh.vertices();
      vbo.set_data(vertices.data(), sizeof(Vertex) * vertices.size(), 0);

      const auto& mat = mesh.material();
      shader->set_vec3("material.ambient", mat.ambient);
      shader->set_vec3("material.diffuse", mat.diffuse);
      shader->set_vec3("material.specular", mat.specular);
      shader->set_float("material.shininess", mat.shininess);
      shader->set_float("material.alpha", mat.alpha);

      const auto& render_config = pobj->get_render_config();
      if (render_config.use_indices)
      {
        const std::vector<GLuint>& indices = mesh.faces_as_indices();
        auto& ebo = rec.ebo;
        ebo.set_data(indices.data(), sizeof(GLuint) * indices.size(), 0);
        glDrawElements(render_config.mode, (GLsizei)(indices.size()), GL_UNSIGNED_INT, nullptr);
      }
      else
      {
        glDrawArrays(render_config.mode, 0, (GLsizei)vertices.size());
      }
      // disable stencil buffer writing
      glStencilFunc(GL_ALWAYS, 0, 0xFF);
      glStencilMask(0x00);
      for (int i = 0; i < 4; i++)
      {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
      }
    }
  }
  shader->unbind();
}

void SceneRenderer::render_selected_objects()
{
  Shader* shader = &ShaderStorage::get(ShaderStorage::ShaderType::OUTLINING);
  shader->bind();

  //glDisable(GL_DEPTH_TEST);
  glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
  glStencilMask(0x00);
  PipelineBuffersRecord& rec = PipelineBuffersManager::instance().get(PIPELINE_BUFFERS_MAIN_RECORD_NAME.data());
  BindChainFIFO bind_chain({ &rec.vao, &rec.vbo, &rec.ebo });
  for (auto& pobj : m_drawables)
  {
    if (pobj->is_selected() && pobj->has_surface())
    {
      const glm::vec3 old_scale = pobj->scale();
      pobj->scale(glm::vec3(old_scale + 0.05f));
      shader->set_matrix4f("modelMatrix", pobj->model_matrix());
      const size_t mesh_count = pobj->mesh_count();
      for (size_t i = 0; i < mesh_count; i++)
      {
        const Mesh& mesh = pobj->get_mesh(i);
        auto& vbo = rec.vbo;
        const std::vector<Vertex>& vertices = mesh.vertices();
        vbo.set_data(vertices.data(), sizeof(Vertex) * vertices.size(), 0);
        const auto& render_config = pobj->get_render_config();
        if (render_config.use_indices)
        {
          const std::vector<GLuint>& indices = mesh.faces_as_indices();
          auto& ebo = rec.ebo;
          ebo.set_data(indices.data(), sizeof(GLuint) * indices.size(), 0);
          glDrawElements(render_config.mode, (GLsizei)(indices.size()), GL_UNSIGNED_INT, nullptr);
        }
        else
        {
          glDrawArrays(render_config.mode, 0, (GLsizei)vertices.size());
        }
      }
      pobj->scale(glm::vec3(old_scale));
    }
  }
  glStencilMask(0xFF);
  glStencilFunc(GL_ALWAYS, 0, 0xFF);
  glEnable(GL_DEPTH_TEST);
  shader->unbind();
}

void SceneRenderer::render_lines()
{
  Shader* shader = &ShaderStorage::get(ShaderStorage::ShaderType::LINES);
  shader->bind();
  shader->set_vec3("lineColor", glm::vec3(0.f, 1.f, 0.f));
  PipelineBuffersRecord& rec = PipelineBuffersManager::instance().get(PIPELINE_BUFFERS_MAIN_RECORD_NAME.data());
  BindChainFIFO bind_chain({ &rec.vao, &rec.vbo, &rec.ebo });
  auto& vbo = rec.vbo;
  auto& ebo = rec.ebo;
  for (const auto& pobj : m_drawables)
  {
    if (pobj->is_bbox_visible())
    {
      shader->set_matrix4f("modelMatrix", pobj->model_matrix());
      // tmp. need better way of handling any geometry change
      if (pobj->bbox().is_empty())
      {
        pobj->calculate_bbox();
      }
      const auto& bbox = pobj->bbox();
      std::array<glm::vec3, 8> bbox_points = bbox.points();
      std::array<Vertex, 8> converted;
      for (size_t i = 0; i < 8; i++)
      {
        converted[i] = Vertex(bbox_points[i]);
      }
      const auto& indices = bbox.lines_indices();
      vbo.set_data(converted.data(), sizeof(Vertex) * 8, 0);
      ebo.set_data(indices.data(), sizeof(GLuint) * indices.size(), 0);
      glDrawElements(GL_LINES, (GLsizei)indices.size(), GL_UNSIGNED_INT, nullptr);
    }
  }
  shader->unbind();
}

void SceneRenderer::render_normals()
{
  struct RenderNormalsCommand
  {
    glm::mat4 modelMatrix;
  };
  static_assert(sizeof(RenderNormalsCommand) == 64);

  static std::vector<RenderNormalsCommand> commands;
  static std::vector<GLsizei> voffsets;
  static std::vector<GLsizei> vcounts;
  PipelineBuffersRecord& rec = PipelineBuffersManager::instance().get(PIPELINE_BUFFERS_NORMALS_RECORD_NAME.data());
  BindChainFIFO bind_chain({ &rec.vao, &rec.vbo });

  if (m_need_normal_data_update)
  {
    commands.clear();
    voffsets.clear();
    vcounts.clear();
    commands.reserve(m_drawables.size());
    voffsets.reserve(m_drawables.size());
    vcounts.reserve(m_drawables.size());
    size_t offset_bytes = 0;

    for (const Object3D* drawable : m_objects_with_visible_normals)
    {
      if (drawable->is_normals_visible())
      {
        const size_t mesh_count = drawable->mesh_count();
        for (size_t i = 0; i < mesh_count; i++)
        {
          const Mesh& mesh = drawable->get_mesh(i);
          const size_t vsize_bytes = sizeof(Vertex) * mesh.vertices().size();
          rec.vbo.set_data(mesh.vertices().data(), vsize_bytes, offset_bytes);
          voffsets.push_back(static_cast<GLsizei>(offset_bytes / sizeof(Vertex)));
          vcounts.push_back(static_cast<GLsizei>(mesh.vertices().size()));
          offset_bytes += vsize_bytes;
          commands.emplace_back(RenderNormalsCommand{ drawable->model_matrix() });
        }
      }
    }

    const size_t commands_size = sizeof(RenderNormalsCommand) * commands.size();
    SSBO& ssbo = rec.ssbos[0];
    if (ssbo.get_size() < commands_size)
    {
      ssbo.resize(commands_size);
    }
    ssbo.bind();
    ssbo.set_data(commands.data(), commands_size, 0);
    ssbo.unbind();

    m_need_normal_data_update = false;
  }

  if (commands.empty())
    return;

  Shader* shader = &ShaderStorage::get(ShaderStorage::ShaderType::NORMALS);
  shader->bind();
  shader->set_vec3("normalColor", glm::vec3(0, 1, 1));
  glMultiDrawArrays(GL_POINTS, voffsets.data(), vcounts.data(), static_cast<GLsizei>(commands.size()));
  shader->unbind();
}

void SceneRenderer::render_skybox()
{
  glDepthFunc(GL_LEQUAL);
  Shader* shader = &ShaderStorage::get(ShaderStorage::ShaderType::SKYBOX);
  shader->bind();
  shader->set_matrix4f("viewMatrix", m_camera.view_matrix());
  shader->set_matrix4f("projectionMatrix", m_camera.get_projection_matrix());
  glActiveTexture(GL_TEXTURE0);
  m_skybox.render();
  shader->unbind();
  glDepthFunc(GL_LESS);
}

void SceneRenderer::handle_mouse_click(int button, int x, int y)
{
  if (button == GLFW_MOUSE_BUTTON_LEFT)
  {
    y = static_cast<int>(m_camera.get_screen_size().y) - y;
    Ray ray = m_camera.cast_ray(x, y);
    std::pair<Object3D*, float> hit_info = { nullptr, INFINITY };
#if DEBUG_RAY
    std::vector<Polyline> rays;
#endif
    for (const auto& d : m_drawables)
    {
      const glm::mat4 inv_model_mat = glm::inverse(d->model_matrix());
      Ray ray_local = Ray(inv_model_mat * glm::vec4(ray.get_origin(), 1), inv_model_mat * glm::vec4(ray.get_direction(), 0));
      if (auto hit = d->hit(ray_local))
      {
        const glm::vec3 ray_hit_pos_world = d->model_matrix() * glm::vec4(hit->position, 1);
#if DEBUG_RAY
        Polyline poly;
        poly.add(ray.get_origin());
        poly.add(ray_hit_pos_world);
        //auto dist = glm::distance(ray.get_origin(), glm::vec3(d->model_matrix() * glm::vec4(hit->position, 1)));
        //std::cout << "dist " << d->name() << " " << dist << '\n';
        rays.push_back(poly);
#endif
        // calculate distance in world space and find closest object. my guess is that we cannot use rayhit->distance here, because it's in local space,
        // therefore, if ray instersects multiple objects, the object that is further may have less distance value than the object that is closer.
        const float hit_distance_world = glm::distance2(ray.get_origin(), ray_hit_pos_world);
        if (hit_info.second > hit_distance_world)
        {
          hit_info.second = hit_distance_world;
          hit_info.first = d.get();
        }
      }
    }
    if (hit_info.first)
    {
      select_object(hit_info.first, false);
    }
#if DEBUG_RAY
    for (auto& r : rays)
    {
      scene.m_drawables.push_back(std::make_unique<Polyline>(r));
    }
#endif
  }
}

void SceneRenderer::handle_window_size_change(int width, int height)
{
  // to avoid crash when window is completely minimized
  if (width == 0 || height == 0)
    return;
  m_window->set_width(width);
  m_window->set_height(height);
  m_camera.set_screen_size({ width, height });
  for (auto& [name, fbo] : m_fbos)
  {
    fbo.bind();
    // resize texture and render buffer
    fbo.attach_texture(width, height, fbo.texture()->internal_fmt(), fbo.texture()->format(), fbo.texture()->pixel_data_type());
    fbo.attach_renderbuffer(width, height, fbo.rb_internal_format(), fbo.rb_attachment());
    fbo.unbind();
  }
  glViewport(0, 0, width, height);
}

void SceneRenderer::handle_keyboard_input(KeyboardHandler::InputKey key, KeyboardHandler::KeyState state)
{
  using Key = KeyboardHandler::InputKey;
  using State = KeyboardHandler::KeyState;
  // pressed once. if we want to know if key is being held, then need to use KeyboardHandler::get_pressed_keys() each frame
  if (state == State::PRESSED)
  {
    if (key == Key::ESC)
    {
      assert(m_selected_objects.empty() || m_selected_objects.size() == 1);
      for (Object3D* obj : m_selected_objects)
      {
        m_selected_objects.pop_back();
        obj->select(false);
      }
    }
  }
  else if (state == State::RELEASED)
  {
    if (key == Key::LEFT_SHIFT)
    {
      m_camera.unfreeze();
      glfwSetInputMode(m_window->gl_window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
  }
}

void SceneRenderer::handle_visible_normals_click(Object3D* obj, bool is_selected)
{
  if (is_selected)
  {
    assert(m_objects_with_visible_normals.count(obj) == 0);
    m_objects_with_visible_normals.insert(obj);
  }
  else
  {
    assert(m_objects_with_visible_normals.count(obj) != 0);
    m_objects_with_visible_normals.erase(obj);
  }
  m_need_normal_data_update = true;
}

void SceneRenderer::prepare_pipeline_buffers()
{
  size_t total_vertices = 0;
  size_t total_indices = 0;
  size_t total_visible_normals = 0;
  for (const auto& drawable : m_drawables)
  {
    const size_t mesh_count = drawable->mesh_count();
    for (size_t i = 0; i < mesh_count; i++)
    {
      const Mesh& mesh = drawable->get_mesh(i);
      total_vertices += mesh.vertices().size();
      if (drawable->get_render_config().use_indices)
      {
        total_indices += mesh.faces_as_indices().size();
      }
      if (drawable->is_normals_visible())
      {
        total_visible_normals += mesh.vertices().size();
      }
    }
  }

  // setup main buffers
  PipelineBuffersRecord& main_record = PipelineBuffersManager::instance().get(PIPELINE_BUFFERS_MAIN_RECORD_NAME.data());
  if (total_vertices * sizeof(Vertex) > main_record.vbo.get_size())
  {
    main_record.vao.bind();
    main_record.vbo.bind();
    main_record.vbo.resize(total_vertices * sizeof(Vertex));
    main_record.vao.unbind();
    main_record.vbo.unbind();
  }
  if (total_indices * sizeof(Vertex) > main_record.ebo.get_size())
  {
    main_record.vao.bind();
    main_record.ebo.bind();
    main_record.ebo.resize(total_indices * sizeof(Vertex));
    main_record.vao.unbind();
    main_record.ebo.unbind();
  }

  // setup buffers for other passes
  auto& normals_record = PipelineBuffersManager::instance().get(PIPELINE_BUFFERS_NORMALS_RECORD_NAME.data());
  if (total_visible_normals * sizeof(Vertex) > normals_record.vbo.get_size())
  {
    normals_record.vao.bind();
    normals_record.vbo.bind();
    normals_record.vbo.resize(total_visible_normals * sizeof(Vertex));
    normals_record.vao.unbind();
    normals_record.vbo.unbind();
  }
}

void SceneRenderer::create_scene()
{
  Vertex arr[6];
  arr[0].position = glm::vec3(0.f, 1.f, 0.f), arr[0].color = glm::vec4(0.f, 1.f, 0.f, 1.f);
  arr[1].position = glm::vec3(0.f, 0.f, 0.f), arr[1].color = glm::vec4(0.f, 1.f, 0.f, 1.f);
  arr[2].position = glm::vec3(0.f, 0.f, 0.f), arr[2].color = glm::vec4(1.f, 0.f, 0.f, 1.f);
  arr[3].position = glm::vec3(1.f, 0.f, 0.f), arr[3].color = glm::vec4(1.f, 0.f, 0.f, 1.f);
  arr[4].position = glm::vec3(0.f, 0.f, 0.f), arr[4].color = glm::vec4(0.f, 0.f, 1.f, 1.f);
  arr[5].position = glm::vec3(0.f, 0.f, 1.f), arr[5].color = glm::vec4(0.f, 0.f, 1.f, 1.f);
  std::unique_ptr<Polyline> origin = std::make_unique<Polyline>();
  for (int i = 0; i < 6; i++) {
    origin->add(arr[i]);
  }
  m_drawables.push_back(std::move(origin));

  Icosahedron* sun = static_cast<Icosahedron*>(m_drawables.emplace_back(std::make_unique<Icosahedron>()).get());
  sun->light_source(true);
  sun->translate(glm::vec3(0.f, 0.5f, 2.f));
  sun->set_color(glm::vec4(1.f, 1.f, 0.f, 1.f));
  sun->scale(glm::vec3(0.3f));
  sun->subdivide_triangles(4);
  sun->project_points_on_sphere();
  m_light_sources.insert(sun);

  Icosahedron* sphere = static_cast<Icosahedron*>(m_drawables.emplace_back(std::make_unique<Icosahedron>()).get());
  sphere->translate(glm::vec3(2.5f, 0.5f, 2.f));
  sphere->set_color(glm::vec4(1.f, 0.f, 0.f, 1.f));
  sphere->subdivide_triangles(4);
  sphere->project_points_on_sphere();
  sphere->scale(glm::vec3(0.3f));
  sphere->apply_shading(Object3D::ShadingMode::SMOOTH_SHADING);

  std::unique_ptr<Cube> c = std::make_unique<Cube>();
  c->translate(glm::vec3(0.25f));
  c->scale(glm::vec3(0.5f));
  c->apply_shading(Object3D::ShadingMode::FLAT_SHADING);
  c->get_mesh(0).set_texture(std::make_shared<Texture2D>(std::string(".\\.\\src\\textures\\brick.jpg")), TextureType::GENERIC);
  m_drawables.push_back(std::move(c));

  std::unique_ptr<Cube> c2 = std::make_unique<Cube>();
  c2->translate(glm::vec3(1.25f, 1.f, 1.f));
  c2->set_color(glm::vec4(0.4f, 1.f, 0.4f, 1.f));
  c2->apply_shading(Object3D::ShadingMode::FLAT_SHADING);
  c2->visible_normals(true);
  m_drawables.push_back(std::move(c2));

  std::unique_ptr<Pyramid> pyr = std::make_unique<Pyramid>();
  pyr->translate(glm::vec3(0.75f, 0.65f, 2.25f));
  pyr->scale(glm::vec3(0.5f));
  pyr->set_color(glm::vec4(0.976f, 0.212f, 0.98f, 1.f));
  pyr->apply_shading(Object3D::ShadingMode::FLAT_SHADING);
  m_drawables.push_back(std::move(pyr));

  std::unique_ptr<BezierCurve> bc = std::make_unique<BezierCurve>(BezierCurve::Type::Quadratic);
  bc->set_start_point(Vertex());
  bc->set_end_point(Vertex(2.5f, 0.f, 0.f));
  bc->set_control_points({ Vertex(1.25f, 2.f, 0.f) });
  bc->set_color(glm::vec4(1.f, 0.f, 0.f, 1.f));
  m_drawables.push_back(std::move(bc));

  std::unique_ptr<BezierCurve> bc2 = std::make_unique<BezierCurve>(BezierCurve::Type::Cubic);
  bc2->set_start_point(Vertex());
  bc2->set_end_point(Vertex(0.f, 0.f, -2.5f));
  bc2->set_control_points({ Vertex(0.f, 2.f, -1.25f), Vertex {0.f, -2.f, -1.75} });
  m_drawables.push_back(std::move(bc2));
}

void SceneRenderer::select_object(Object3D* obj, bool click_from_menu_item)
{
  // do not select object that is behind imgui's menu, but select it when we are clicking on obj name inside menu itself
  ImGuiIO& io = ImGui::GetIO();
  if (!click_from_menu_item && io.WantCaptureMouse)
    return;
  if (obj->is_selected())
    return;
  // for now support only single object selection
  assert(m_selected_objects.size() == 0 || m_selected_objects.size() == 1);
  std::cout << obj->get_name() << " selected\n";
  if (m_selected_objects.size())
  {
    m_selected_objects.back()->select(false);
    m_selected_objects.pop_back();
  }
  m_selected_objects.push_back(obj);
  obj->select(true);
}

void SceneRenderer::tick()
{
  ImGuiIO& io = ImGui::GetIO();
  m_camera.scale_speed(io.DeltaTime);
  for (auto& obj : m_drawables) 
  {
    obj->set_delta_time(io.DeltaTime);
    if (obj->is_rotating())
    {
      obj->rotate(obj->m_rotation_angle, obj->m_rotation_axis);
    }
    if (obj->is_normals_visible() && m_objects_with_visible_normals.count(obj.get()) == 0)
    {
      m_objects_with_visible_normals.insert(obj.get());
      m_need_normal_data_update = true;
    }
  }

  m_cam_controller.tick();
  KeyboardHandler* kh = static_cast<KeyboardHandler*>(m_window->get_input_handler(UserInputHandler::KEYBOARD));
  if (kh->get_keystate(KeyboardHandler::InputKey::LEFT_SHIFT) == KeyboardHandler::PRESSED)
  {
    m_camera.freeze();
    glfwSetInputMode(m_window->gl_window(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  }

  double x, y;
  glfwGetCursorPos(m_window->gl_window(), &x, &y);
  // update virtual cursor pos to avoid camera jumps after cursor goes out of window or window regains focus,
  // because once cursor goes out of glfw window cursor callback is no longer triggered
  UserInputHandler* h = m_window->get_input_handler(UserInputHandler::CURSOR_POSITION);
  static_cast<CursorPositionHandler*>(h)->update_current_pos(x, y);

  PipelineBuffersRecord& rec = PipelineBuffersManager::instance().get(PIPELINE_BUFFERS_MAIN_RECORD_NAME.data());
  rec.ubos[0].bind();
  rec.ubos[0].set_data(&m_camera.view_matrix(), sizeof(glm::mat4), 0);
  rec.ubos[0].set_data(&m_camera.get_projection_matrix(), sizeof(glm::mat4), sizeof(glm::mat4));
  rec.ubos[0].unbind();

  prepare_pipeline_buffers();
}

namespace
{
  void setup_opengl()
  {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
}
