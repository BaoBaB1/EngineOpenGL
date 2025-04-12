#include "SceneRenderer.hpp"
#include "WindowGLFW.hpp"
#include "Shader.hpp"
#include "opengl/VertexArrayObject.hpp"
#include "opengl/VertexBufferObject.hpp"
#include "opengl/ElementBufferObject.hpp"
#include "Debug.hpp"
#include "Camera.hpp"
#include "input/KeyboardHandler.hpp"
#include "input/CursorPositionHandler.hpp"
#include "input/MouseInputHandler.hpp"
#include "ShaderStorage.hpp"
#include "BindGuard.hpp"
#include "PipelineBufferManager.hpp"
#include "Logger.hpp"
#include "ge/Cube.hpp"
#include "ge/Icosahedron.hpp"
#include "ge/Polyline.hpp"
#include "ge/Pyramid.hpp"
#include "ge/BezierCurve.hpp"
#include "ge/Skybox.hpp"
#include "ge/GeometryFactory.hpp"

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
#include <fstream>

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
}

namespace fury
{
  SceneRenderer::SceneRenderer(WindowGLFW* window) : m_window(window)
  {
    const int w = window->width();
    const int h = window->height();
    m_ui.init(this);
    SceneInfo* scene_info_component = m_ui.get_component<SceneInfo>("SceneInfo");
    scene_info_component->on_polygon_mode_change += new InstanceListener(this, &SceneRenderer::change_polygon_mode);
    m_camera.set_screen_size({ w, h });
    m_camera.set_position(glm::vec3(-4.f, 2.f, 3.f));
    m_camera.look_at(glm::vec3(2.f, 0.5f, 0.5f));

    CursorPositionHandler* cursor_pos_handler = static_cast<CursorPositionHandler*>(m_window->get_input_handler(UserInputHandler::CURSOR_POSITION));
    MouseInputHandler* mouse_input_handler = static_cast<MouseInputHandler*>(m_window->get_input_handler(UserInputHandler::MOUSE_INPUT));
    KeyboardHandler* keyboard_input_handler = static_cast<KeyboardHandler*>(m_window->get_input_handler(UserInputHandler::KEYBOARD));
    mouse_input_handler->on_button_click += new InstanceListener(this, &SceneRenderer::handle_mouse_click);
    keyboard_input_handler->on_key_state_change += new InstanceListener(this, &SceneRenderer::handle_keyboard_input);
    m_window->on_window_size_change += new InstanceListener(this, &SceneRenderer::handle_window_size_change);
    m_cam_controller.init(&m_camera, keyboard_input_handler, cursor_pos_handler);
    ShaderStorage::init();

    auto& main_scene_fbo = m_fbos["main"];
    main_scene_fbo.bind();
    main_scene_fbo.attach_texture(w, h, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE);
    main_scene_fbo.attach_renderbuffer(w, h, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL_ATTACHMENT);
    main_scene_fbo.unbind();

    m_skybox = Cubemap(skybox_faces);
    m_screen_quad.set_texture_id(main_scene_fbo.texture()->id());

    UniformBuffer& ubo = PipelineUBOManager::get("cameraData");
    ubo.resize(sizeof(glm::mat4) * 2);
    ubo.set_binding_point(0);

    ::setup_opengl();
    create_scene();
    m_render_passes.emplace_back(std::make_unique<GeometryPass>(this));
    m_render_passes.emplace_back(std::make_unique<NormalsPass>(this));
    m_render_passes.emplace_back(std::make_unique<LinesPass>(this));
    for (auto& rp : m_render_passes)
    {
      rp->update();
    }
    for (const auto& [name, fbo] : m_fbos)
    {
      fbo.bind();
      assert(fbo.is_complete());
      fbo.unbind();
    }
    for (auto& drawable : m_drawables)
    {
      // init bbox and center
      drawable->update();
    }
  }

  SceneRenderer::~SceneRenderer()
  {
  }

  void SceneRenderer::render()
  {
    const auto& main_fbo = m_fbos.at("main");
    GLFWwindow* gl_window = m_window->gl_window();
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
      for (auto& rp : m_render_passes)
      {
        rp->tick();
      }
      m_ui.tick();
      main_fbo.unbind();
      m_screen_quad.tick();
      glfwSwapBuffers(gl_window);
    }
  }

  void SceneRenderer::save(const std::string& file) const
  {
    std::ofstream ofs(file, std::ios_base::binary);
    if (!ofs.is_open())
    {
      Logger::error("Save failed to open file {}.", file);
      return;
    }
    ofs.write(reinterpret_cast<const char*>(&m_polygon_mode), sizeof(GLint));
    ofs.write(reinterpret_cast<const char*>(&m_camera), sizeof(Camera));
    const size_t drawables_count = m_drawables.size();
    ofs.write(reinterpret_cast<const char*>(&drawables_count), sizeof(size_t));
    for (const auto& drawable : m_drawables)
    {
      drawable->write(ofs);
    }
    ofs.close();
  }

  void SceneRenderer::load(const std::string& file)
  {
    std::ifstream ifs(file, std::ios_base::binary);
    if (!ifs.is_open())
    {
      Logger::error("Load failed to open file {}.", file);
      return;
    }

    // cleanup current scene
    m_drawables.clear();
    m_selected_objects.clear();
    m_light_sources.clear();

    ifs.read(reinterpret_cast<char*>(&m_polygon_mode), sizeof(GLint));
    ifs.read(reinterpret_cast<char*>(&m_camera), sizeof(Camera));
    size_t drawables_count = 0;
    ifs.read(reinterpret_cast<char*>(&drawables_count), sizeof(size_t));
    m_drawables.reserve(drawables_count);
    for (size_t i = 0; i < drawables_count; i++)
    {
      int32_t obj_type;
      ifs.read(reinterpret_cast<char*>(&obj_type), sizeof(int32_t));
      std::unique_ptr<Object3D> obj = GeometryFactory::create_from_type(obj_type);
      obj->read(ifs);
      m_drawables.push_back(std::move(obj));
    }
    ifs.close();

    // fill scene's structs
    for (size_t i = 0; i < m_drawables.size(); i++)
    {
      auto& drawable = m_drawables[i];
      if (drawable->is_light_source())
      {
        m_light_sources.insert(i);
      }
      if (drawable->is_selected())
      {
        m_selected_objects.push_back(drawable.get());
      }
      // calculate object center + bbox
      drawable->update();
    }

    // update render passes in accordance with the new scene
    for (auto& render_pass : m_render_passes)
    {
      render_pass->update();
    }
    
    // fix to avoid camera jumps ???
    // again, cursor pos in handler can have huge offset difference at this point
    static_cast<CursorPositionHandler*>(m_window->get_input_handler(UserInputHandler::CURSOR_POSITION))->update_ignore_frames();
  }

  void SceneRenderer::render_skybox()
  {
    glDepthFunc(GL_LEQUAL);
    Shader* shader = &ShaderStorage::get(ShaderStorage::ShaderType::SKYBOX);
    shader->bind();
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
            hit_info.first = const_cast<Object3D*>(d.get());
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
      else if (key == Key::LEFT_SHIFT)
      {
        m_camera.freeze();
        glfwSetInputMode(m_window->gl_window(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
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

  void SceneRenderer::change_polygon_mode(int new_mode)
  {
    m_polygon_mode = new_mode;
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
    auto origin = std::make_unique<Polyline>();
    for (int i = 0; i < 6; i++) {
      origin->add(arr[i]);
    }
    m_drawables.push_back(std::move(origin));

    auto& sun = std::make_unique<Icosahedron>();
    sun->light_source(true);
    sun->translate(glm::vec3(0.f, 0.5f, 2.f));
    sun->set_color(glm::vec4(1.f, 1.f, 0.f, 1.f));
    sun->set_is_fixed_shading(true);
    sun->scale(glm::vec3(0.3f));
    sun->subdivide_triangles(4);
    sun->project_points_on_sphere();
    m_drawables.push_back(std::move(sun));
    m_light_sources.insert(1);

    auto& sphere = std::make_unique<Icosahedron>();
    sphere->translate(glm::vec3(2.5f, 0.5f, 2.f));
    sphere->set_color(glm::vec4(1.f, 0.f, 0.f, 1.f));
    sphere->subdivide_triangles(4);
    sphere->project_points_on_sphere();
    sphere->scale(glm::vec3(0.3f));
    sphere->apply_shading(Object3D::ShadingMode::SMOOTH_SHADING);
    m_drawables.push_back(std::move(sphere));

    auto& c = std::make_unique<Cube>();
    c->translate(glm::vec3(0.25f));
    c->scale(glm::vec3(0.5f));
    c->apply_shading(Object3D::ShadingMode::FLAT_SHADING);
    c->get_mesh(0).set_texture(std::make_shared<Texture2D>(std::string(".\\.\\src\\textures\\brick.jpg")), TextureType::GENERIC);
    m_drawables.push_back(std::move(c));

    auto& c2 = std::make_unique<Cube>();
    c2->translate(glm::vec3(1.25f, 1.f, 1.f));
    c2->set_color(glm::vec4(0.4f, 1.f, 0.4f, 1.f));
    c2->apply_shading(Object3D::ShadingMode::FLAT_SHADING);
    c2->visible_normals(true);
    m_drawables.push_back(std::move(c2));

    auto& pyr = std::make_unique<Pyramid>();
    pyr->translate(glm::vec3(0.75f, 0.65f, 2.25f));
    pyr->scale(glm::vec3(0.5f));
    pyr->set_color(glm::vec4(0.976f, 0.212f, 0.98f, 1.f));
    pyr->apply_shading(Object3D::ShadingMode::FLAT_SHADING);
    m_drawables.push_back(std::move(pyr));

    auto& bc = std::make_unique<BezierCurve>(BezierCurveType::Quadratic);
    bc->set_start_point(Vertex());
    bc->set_end_point(Vertex(2.5f, 0.f, 0.f));
    bc->set_control_points({ Vertex(1.25f, 2.f, 0.f) });
    bc->set_color(glm::vec4(1.f, 0.f, 0.f, 1.f));
    m_drawables.push_back(std::move(bc));

    auto& bc2 = std::make_unique<BezierCurve>(BezierCurveType::Cubic);
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
    {
      return;
    }
    if (click_from_menu_item)
    {
      // look at selected object
      m_camera.look_at(obj->center() + glm::vec3(obj->model_matrix()[3]));
    }
    if (obj->is_selected())
    {
      return;
    }
    
    // for now support only single object selection
    assert(m_selected_objects.size() == 0 || m_selected_objects.size() == 1);
    Logger::debug("{} selected", obj->get_name());
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
        obj->rotate(obj->rotation_angle(), obj->rotation_axis());
      }
    }

    m_cam_controller.tick();

    double x, y;
    glfwGetCursorPos(m_window->gl_window(), &x, &y);
    // update virtual cursor pos to avoid camera jumps after cursor goes out of window or window regains focus,
    // because once cursor goes out of glfw window cursor callback is no longer triggered
    UserInputHandler* h = m_window->get_input_handler(UserInputHandler::CURSOR_POSITION);
    static_cast<CursorPositionHandler*>(h)->update_current_pos(x, y);

    UniformBuffer& ubo = PipelineUBOManager::get("cameraData");
    ubo.bind();
    ubo.set_data(&m_camera.view_matrix(), sizeof(glm::mat4), 0);
    ubo.set_data(&m_camera.get_projection_matrix(), sizeof(glm::mat4), sizeof(glm::mat4));
    ubo.unbind();
  }
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
