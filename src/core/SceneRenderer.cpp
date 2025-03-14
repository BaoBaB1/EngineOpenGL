#include "imgui.h"
#include "imgui_internal.h"

#include <cassert>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>

#include "SceneRenderer.hpp"
#include "Ui.hpp"
#include "MainWindow.hpp"
#include "Shader.hpp"
#include "VertexArrayObject.hpp"
#include "VertexBufferObject.hpp"
#include "ElementBufferObject.hpp"
#include "FrameBufferObject.hpp"
#include "Debug.hpp"
#include "Camera.hpp"
#include "KeyboardHandler.hpp"
#include "CursorPositionHandler.hpp"
#include "MouseInputHandler.hpp"
#include "ShaderStorage.hpp"
#include "GPUBuffers.hpp"
#include "ge/Cube.hpp"
#include "ge/Icosahedron.hpp"
#include "ge/Polyline.hpp"
#include "ge/Pyramid.hpp"
#include "ge/BezierCurve.hpp"
#include "ge/Skybox.hpp"

static void setup_opengl();
static void get_desktop_resolution(int& horizontal, int& vertical);

using namespace GlobalState;

SceneRenderer::SceneRenderer()
{
  glfwInit();
  int w = 1600, h = 900;
  //get_desktop_resolution(w, h);

  // fullscreen window
  m_window = std::make_unique<MainWindow>(w, h, "MainWindow");
  m_gpu_buffers = std::make_unique<GPUBuffers>();
  m_ui = std::make_unique<Ui>(*this, m_window.get());
  m_camera.set_position(glm::vec3(-4.f, 2.f, 3.f));
  m_camera.look_at(glm::vec3(2.f, 0.5f, 0.5f));
  m_projection_mat = glm::mat4(1.f);
  m_projection_mat = glm::perspective(glm::radians(45.f), (float)m_window->width() / m_window->height(), 0.1f, 100.f);

  ShaderStorage::init();

  auto main_scene_fbo = FrameBufferObject();
  main_scene_fbo.bind();
  main_scene_fbo.attach_texture(w, h, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE);
  main_scene_fbo.attach_renderbuffer(w, h, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL_ATTACHMENT);
  main_scene_fbo.unbind();
  m_fbos["main"] = std::move(main_scene_fbo);

  auto picking_fbo = FrameBufferObject();
  picking_fbo.bind();
  picking_fbo.attach_texture(w, h, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT);
  picking_fbo.attach_renderbuffer(w, h, GL_DEPTH_COMPONENT, GL_DEPTH_ATTACHMENT);
  picking_fbo.unbind();
  m_fbos["picking"] = std::move(picking_fbo);

  m_skybox_vbo = std::make_unique<VertexBufferObject>();
  m_skybox_vao = std::make_unique<VertexArrayObject>();
  m_skybox_vao->bind();
  m_skybox_vbo->bind();
  m_skybox_vao->link_attrib(0, 3, GL_FLOAT, sizeof(float) * 3, nullptr);
  m_skybox_vbo->set_data(skyboxVertices, sizeof(skyboxVertices));
  m_skybox_vao->unbind();
  m_skybox_vbo->unbind();
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
  const auto& picking_fbo = m_fbos.at("picking");
  ScreenQuad screen_quad(main_fbo.texture()->id());

  const std::string skybox_folder = ".\\.\\src\\textures\\skybox\\";
  std::array<std::string, 6> skybox_faces =
  { 
    skybox_folder + "right.jpg",
    skybox_folder + "left.jpg",
    skybox_folder + "top.jpg",
    skybox_folder + "bottom.jpg",
    skybox_folder + "front.jpg",
    skybox_folder + "back.jpg" 
  };
  Skybox skybox(Cubemap(std::move(skybox_faces)));

  m_gpu_buffers->bind_all();
  auto& vao = m_gpu_buffers->vao;
  vao->link_attrib(0, 3, GL_FLOAT, sizeof(Vertex), nullptr);                        // position
  vao->link_attrib(1, 3, GL_FLOAT, sizeof(Vertex), (void*)(sizeof(GLfloat) * 3));   // normal
  vao->link_attrib(2, 4, GL_FLOAT, sizeof(Vertex), (void*)(sizeof(GLfloat) * 6));   // color
  vao->link_attrib(3, 2, GL_FLOAT, sizeof(Vertex), (void*)(sizeof(GLfloat) * 10));  // texture
  m_gpu_buffers->bind_all();
  
  while (!glfwWindowShouldClose(gl_window))
  {
    glfwPollEvents();
    new_frame_update();
    handle_input();
    glPolygonMode(GL_FRONT_AND_BACK, m_polygon_mode);

    picking_fbo.bind();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    render_picking_fbo();
    picking_fbo.unbind();

    // render to a custom framebuffer
    main_fbo.bind();
    glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    // render scene before gui to make sure that imgui window always will be on top of drawn entities
    render_skybox(skybox);
    render_scene();
    render_selected_objects();
    render_normals();
    render_lines();
    m_ui->render();
    main_fbo.unbind();

    // set GL_FILL mode because next we are rendering texture
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_DEPTH_TEST);
    screen_quad.render();

    glfwSwapBuffers(gl_window);
  }
}

void SceneRenderer::render_scene()
{
  Shader* shader = &ShaderStorage::get(ShaderStorage::ShaderType::MAIN);
  shader->bind();
  shader->set_vec3("viewPos", m_camera.position());
  shader->set_matrix4f("viewMatrix", m_camera.view_matrix());
  shader->set_matrix4f("projectionMatrix", m_projection_mat);
  shader->set_int("defaultTexture", 0);
  shader->set_int("ambientTex", 1);
  shader->set_int("diffuseTex", 2);
  shader->set_int("specularTex", 3);
  const VertexLayout vlayout = shader->vertex_layout();
  m_gpu_buffers->bind_all();
  for (const auto& pobj : m_drawables)
  {
    shader->set_matrix4f("modelMatrix", pobj->model_matrix());
    shader->set_bool("applyShading", pobj->m_shading_mode != Object3D::ShadingMode::NO_SHADING && !pobj->is_light_source());

    if (pobj->is_rotating())
    {
      pobj->rotate(pobj->m_rotation_angle, pobj->m_rotation_axis);
    }
    if (pobj->is_selected() && pobj->has_surface())
    {
      // enable writing to the stencil buffer and disable it in the end of current iteration
      glStencilFunc(GL_ALWAYS, 1, 0xFF);
      glStencilMask(0xFF);
    }
    if (pobj->is_light_source())
    {
      // center in world space
      shader->set_vec3("lightPos", pobj->center() + glm::vec3(pobj->m_model_mat[3]));
      shader->set_vec3("lightColor", glm::vec3(1.f));
    }

    const size_t mesh_count = pobj->mesh_count();
    for (size_t i = 0; i < mesh_count; i++)
    {
      const Mesh& mesh = pobj->get_mesh(i);

      // bind textures
      if (auto tex = mesh.get_texture(TextureType::UNKNOWN))
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

      auto& vbo = m_gpu_buffers->vbo;
      const std::vector<Vertex>& vertices = mesh.vertices();
      vbo->set_data(vertices.data(), sizeof(Vertex) * vertices.size());

      const auto mat = mesh.material();
      shader->set_bool("hasMaterial", bool(mat));
      if (mat)
      {
        shader->set_vec3("material.ambient", mat->ambient);
        shader->set_vec3("material.diffuse", mat->diffuse);
        shader->set_vec3("material.specular", mat->specular);
        shader->set_float("material.shininess", mat->shininess);
        shader->set_float("material.alpha", mat->alpha);
      }

      const auto& render_config = pobj->get_render_config();
      if (render_config.use_indices)
      {
        const std::vector<GLuint>& indices = mesh.faces_as_indices();
        auto& ebo = m_gpu_buffers->ebo;
        ebo->set_data(indices.data(), sizeof(GLuint) * indices.size());
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
  m_gpu_buffers->unbind_all();
  shader->unbind();
}

void SceneRenderer::render_picking_fbo()
{
  glEnable(GL_DEPTH_TEST);
  Shader* shader = &ShaderStorage::get(ShaderStorage::ShaderType::PICKING);
  shader->bind();
  shader->set_matrix4f("viewMatrix", m_camera.view_matrix());
  shader->set_matrix4f("projectionMatrix", m_projection_mat);
  const VertexLayout vlayout = shader->vertex_layout();
  m_gpu_buffers->bind_all();
  for (size_t i = 0; i < m_drawables.size(); i++)
  {
    const auto& drawable = m_drawables[i];
    shader->set_matrix4f("modelMatrix", drawable->model_matrix());
    shader->set_uint("objectIndex", i + 1);
    const size_t mesh_count = drawable->mesh_count();
    for (size_t j = 0; j < mesh_count; j++)
    {
      const Mesh& mesh = drawable->get_mesh(j);
      auto& vbo = m_gpu_buffers->vbo;
      const std::vector<Vertex>& vertices = mesh.vertices();
      vbo->set_data(vertices.data(), sizeof(Vertex) * vertices.size());
      const auto& render_config = drawable->get_render_config();
      if (render_config.use_indices)
      {
        const std::vector<GLuint>& indices = mesh.faces_as_indices();
        auto& ebo = m_gpu_buffers->ebo;
        ebo->set_data(indices.data(), sizeof(GLuint) * indices.size());
        glDrawElements(render_config.mode, (GLsizei)(indices.size()), GL_UNSIGNED_INT, nullptr);
      }
      else
      {
        glDrawArrays(render_config.mode, 0, (GLsizei)vertices.size());
      }
    }
  }
  m_gpu_buffers->unbind_all();
  shader->unbind();
  glDisable(GL_DEPTH_TEST);
}

void SceneRenderer::render_selected_objects()
{
  Shader* shader = &ShaderStorage::get(ShaderStorage::ShaderType::OUTLINING);
  shader->bind();
  shader->set_matrix4f("viewMatrix", m_camera.view_matrix());
  shader->set_matrix4f("projectionMatrix", m_projection_mat);
  const VertexLayout vlayout = shader->vertex_layout();

  //glDisable(GL_DEPTH_TEST);
  glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
  glStencilMask(0x00);
  m_gpu_buffers->bind_all();
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
        auto& vbo = m_gpu_buffers->vbo;
        const std::vector<Vertex>& vertices = mesh.vertices();
        vbo->set_data(vertices.data(), sizeof(Vertex) * vertices.size());
        const auto& render_config = pobj->get_render_config();
        if (render_config.use_indices)
        {
          const std::vector<GLuint>& indices = mesh.faces_as_indices();
          auto& ebo = m_gpu_buffers->ebo;
          ebo->set_data(indices.data(), sizeof(GLuint) * indices.size());
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
  m_gpu_buffers->unbind_all();
  glStencilMask(0xFF);
  glStencilFunc(GL_ALWAYS, 0, 0xFF);
  glEnable(GL_DEPTH_TEST);
  shader->unbind();
}

void SceneRenderer::render_lines()
{
  Shader* shader = &ShaderStorage::get(ShaderStorage::ShaderType::LINES);
  shader->bind();
  shader->set_matrix4f("viewMatrix", m_camera.view_matrix());
  shader->set_matrix4f("projectionMatrix", m_projection_mat);
  shader->set_vec3("lineColor", glm::vec3(0.f, 1.f, 0.f));
  const VertexLayout vlayout = shader->vertex_layout();
  auto& vbo = m_gpu_buffers->vbo;
  auto& ebo = m_gpu_buffers->ebo;
  m_gpu_buffers->bind_all();
  for (const auto& pobj : m_drawables)
  {
    shader->set_matrix4f("modelMatrix", pobj->model_matrix());
    if (pobj->is_bbox_visible())
    {
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
      auto indices = bbox.lines_indices();
      vbo->set_data(converted.data(), sizeof(Vertex) * 8);
      ebo->set_data(indices.data(), sizeof(GLuint) * indices.size());
      glDrawElements(GL_LINES, (GLsizei)indices.size(), GL_UNSIGNED_INT, nullptr);
    }
  }
  m_gpu_buffers->unbind_all();
  shader->unbind();
}

void SceneRenderer::render_normals()
{
  Shader* shader = &ShaderStorage::get(ShaderStorage::ShaderType::NORMALS);
  shader->bind();
  shader->set_matrix4f("viewMatrix", m_camera.view_matrix());
  shader->set_matrix4f("projectionMatrix", m_projection_mat);
  shader->set_vec3("normalColor", glm::vec3(0, 1, 1));
  const VertexLayout vlayout = shader->vertex_layout();
  m_gpu_buffers->bind_all();
  for (const auto& pobj : m_drawables)
  {
    shader->set_matrix4f("modelMatrix", pobj->model_matrix());
    auto& vbo = m_gpu_buffers->vbo;
    if (pobj->is_normals_visible())
    {
      const size_t mesh_count = pobj->mesh_count();
      for (size_t i = 0; i < mesh_count; i++)
      {
        const Mesh& mesh = pobj->get_mesh(i);
        vbo->set_data(mesh.vertices().data(), sizeof(Vertex) * mesh.vertices().size());
        glDrawArrays(GL_POINTS, 0, mesh.vertices().size());
      }
    }
  }
  m_gpu_buffers->unbind_all();
  shader->unbind();
}

void SceneRenderer::render_skybox(const Skybox& skybox)
{
  glDepthFunc(GL_LEQUAL);
  Shader* shader = &ShaderStorage::get(ShaderStorage::ShaderType::SKYBOX);
  shader->bind();
  shader->set_matrix4f("viewMatrix", m_camera.view_matrix());
  shader->set_matrix4f("projectionMatrix", m_projection_mat);
  glActiveTexture(GL_TEXTURE0);
  m_skybox_vao->bind();
  m_skybox_vbo->bind();
  skybox.get_cubemap().bind();
  glDrawArrays(GL_TRIANGLES, 0, 36);
  skybox.get_cubemap().unbind();
  m_skybox_vbo->unbind();
  m_skybox_vao->unbind();
  shader->unbind();
  glDepthFunc(GL_LESS);
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
  c->set_texture(std::make_shared<Texture2D>(std::string(".\\.\\src\\textures\\brick.jpg")), TextureType::UNKNOWN, 0);
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

void SceneRenderer::select_object(int index, bool click_from_menu_item)
{
  // do not select object that is behind imgui's menu, but select it when we are clicking on obj name inside menu itself
  ImGuiIO& io = ImGui::GetIO();
  if (!click_from_menu_item && io.WantCaptureMouse)
    return;
  if (m_drawables[index]->is_selected())
    return;
  // for now support only single object selection
  assert(m_selected_objects.size() == 0 || m_selected_objects.size() == 1);
  std::cout << m_drawables[index]->name() << " selected\n";
  if (m_selected_objects.size())
  {
    m_drawables[m_selected_objects.back()]->select(false);
    m_selected_objects.pop_back();
  }
  m_selected_objects.push_back(index);
  m_drawables[index]->select(true);
}

void SceneRenderer::new_frame_update()
{
  ImGuiIO& io = ImGui::GetIO();
  m_camera.scale_speed(io.DeltaTime);
  for (auto& obj : m_drawables) 
  {
    obj->set_delta_time(io.DeltaTime);
  }

  double x, y;
  glfwGetCursorPos(m_window->gl_window(), &x, &y);
  // update virtual cursor pos to avoid camera jumps after cursor goes out of window or window regains focus,
  // because once cursor goes out of glfw window cursor callback is no longer triggered
  auto& h = m_window->input_handlers()[1];
  assert(h->type() == UserInputHandler::CURSOR_POSITION);
  static_cast<CursorPositionHandler*>(h.get())->update_current_pos(x, y);
}

void SceneRenderer::handle_input()
{
  // handle pressed key every frame for smooth movement because glfw calls callback not every frame
  KeyboardHandler* kh = static_cast<KeyboardHandler*>(m_window->input_handlers()[0].get());
  assert(kh->type() == UserInputHandler::KEYBOARD);
  if (kh->disabled())
    return;
  using InputKey = KeyboardHandler::InputKey;
  if (kh->get_keystate(InputKey::W) == KeyboardHandler::PRESSED || kh->get_keystate(InputKey::ARROW_UP) == KeyboardHandler::PRESSED)
  {
    m_camera.move(Camera::Direction::FORWARD);
  }
  if (kh->get_keystate(InputKey::A) == KeyboardHandler::PRESSED || kh->get_keystate(InputKey::ARROW_LEFT) == KeyboardHandler::PRESSED)
  {
    m_camera.move(Camera::Direction::LEFT);
  }
  if (kh->get_keystate(InputKey::S) == KeyboardHandler::PRESSED || kh->get_keystate(InputKey::ARROW_DOWN) == KeyboardHandler::PRESSED)
  {
    m_camera.move(Camera::Direction::BACKWARD);
  }
  if (kh->get_keystate(InputKey::D) == KeyboardHandler::PRESSED || kh->get_keystate(InputKey::ARROW_RIGHT) == KeyboardHandler::PRESSED)
  {
    m_camera.move(Camera::Direction::RIGHT);
  }
  if (kh->get_keystate(InputKey::ESC) == KeyboardHandler::PRESSED)
  {
    for (int idx : m_selected_objects)
    {
      assert(m_selected_objects.size() == 1);
      m_selected_objects.pop_back();
      m_drawables[idx]->select(false);
    }
  }
  if (kh->get_keystate(InputKey::SPACE) == KeyboardHandler::PRESSED)
  {
    m_camera.move(Camera::Direction::UP);
  }
  if (kh->get_keystate(InputKey::LEFT_CTRL) == KeyboardHandler::PRESSED)
  {
    m_camera.move(Camera::Direction::DOWN);
  }

  if (kh->get_keystate(InputKey::LEFT_SHIFT) == KeyboardHandler::PRESSED)
  {
    m_camera.freeze();
    glfwSetInputMode(m_window->gl_window(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  }
  else if (kh->get_keystate(InputKey::LEFT_SHIFT) == KeyboardHandler::RELEASED)
  {
    m_camera.unfreeze();
    kh->reset_state(InputKey::LEFT_SHIFT);
    glfwSetInputMode(m_window->gl_window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  }
}

void ScreenQuad::render()
{
  Shader& shader = ShaderStorage::get(ShaderStorage::ShaderType::FBO_DEFAULT);
  shader.bind();
  vao->bind();
  vbo->bind();
  shader.set_int("screenTexture", 0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_tex_id);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  vbo->unbind();
  vao->unbind();
  shader.unbind();
}

static void setup_opengl()
{
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glEnable(GL_STENCIL_TEST);
  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE); 
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void get_desktop_resolution(int& horizontal, int& vertical)
{
  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  auto info = glfwGetVideoMode(monitor);
  horizontal = info->width;
  vertical = info->height;
}
