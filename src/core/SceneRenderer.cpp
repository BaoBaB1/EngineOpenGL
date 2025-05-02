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
#include "utils/Utils.hpp"
#include "AssetManager.hpp"

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

constexpr int SHADOWMAP_WIDTH = 2048;
constexpr int SHADOWMAP_HEIGHT = 2048;

namespace
{
  void setup_opengl();
  constexpr std::array<std::string_view, 6> skybox_faces =
  {
    ".\\.\\assets\\textures\\skybox\\right.jpg",
    ".\\.\\assets\\textures\\skybox\\left.jpg",
    ".\\.\\assets\\textures\\skybox\\top.jpg",
    ".\\.\\assets\\textures\\skybox\\bottom.jpg",
    ".\\.\\assets\\textures\\skybox\\front.jpg",
    ".\\.\\assets\\textures\\skybox\\back.jpg"
  };

  constexpr std::array<float, 24> init_shadow_map_data()
  {
    constexpr float xmin = -1.f;
    constexpr float xmax = -0.4f;
    constexpr float ymin = -1.f;
    constexpr float ymax = -0.15f;
    // bottom left window corner
    constexpr std::array shadow_map_data =
    {
      // positions   // texCoords
      xmin, ymax,  0.0f, 1.0f,
      xmin, ymin,  0.0f, 0.0f,
      xmax, ymin,  1.0f, 0.0f,
      xmin, ymax,  0.0f, 1.0f,
      xmax, ymin,  1.0f, 0.0f,
      xmax, ymax,  1.0f, 1.0f
    };
    return shadow_map_data;
  };
  constexpr std::array shadow_map_data = init_shadow_map_data();
}

namespace fury
{
  SceneRenderer::SceneRenderer(WindowGLFW* window) : m_window(window)
  {
    const int w = window->width();
    const int h = window->height();
    AssetManager::init();
    m_ui.init(this);
    SceneInfo* scene_info_component = m_ui.get_component<SceneInfo>("SceneInfo");
    Gizmo* gizmo_component = m_ui.get_component<Gizmo>("Gizmo");
    FileExplorer* file_explorer_component = m_ui.get_component<FileExplorer>("FileExplorer");
    file_explorer_component->on_open += new InstanceListener(this, &SceneRenderer::handle_file_explorer_opening);
    file_explorer_component->on_close += new InstanceListener(this, &SceneRenderer::handle_file_explorer_closing);
    gizmo_component->on_object_change += new InstanceListener(this, &SceneRenderer::handle_object_change);
    scene_info_component->on_polygon_mode_change += new InstanceListener(this, &SceneRenderer::change_polygon_mode);
    on_new_object_added += new InstanceListener(this, &SceneRenderer::handle_added_object);
    m_camera.set_screen_size({ w, h });
    m_camera.set_position(glm::vec3(-4.f, 2.f, 3.f));
    m_camera.look_at(glm::vec3(2.f, 0.5f, 0.5f));

    CursorPositionHandler* cursor_pos_handler = m_window->get_input_handler<CursorPositionHandler>(UserInputHandler::CURSOR_POSITION);
    MouseInputHandler* mouse_input_handler = m_window->get_input_handler<MouseInputHandler>(UserInputHandler::MOUSE_INPUT);
    KeyboardHandler* keyboard_input_handler = m_window->get_input_handler<KeyboardHandler>(UserInputHandler::KEYBOARD);
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

    auto& shadows_fbo = m_fbos["shadowMap"];
    shadows_fbo.bind();
    shadows_fbo.attach_texture(SHADOWMAP_WIDTH, SHADOWMAP_HEIGHT, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT, false);
    shadows_fbo.texture()->bind();
    // anything that is out of depth map is not in shadow
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    shadows_fbo.texture()->unbind();
    shadows_fbo.unbind();

    for (const auto& [name, fbo] : m_fbos)
    {
      fbo.bind();
      assert(fbo.is_complete());
      fbo.unbind();
    }

    m_skybox.set_cubemap(Cubemap(skybox_faces));
    m_screen_quad.init(main_scene_fbo.texture()->id());
    m_shadow_map_quad.init(shadow_map_data, shadows_fbo.texture()->id(), true);

    UniformBuffer& ubo = PipelineUBOManager::get("cameraData");
    ubo.bind();
    ubo.resize(sizeof(glm::mat4) * 2);
    ubo.set_binding_point(0);
    ubo.unbind();

    ::setup_opengl();
    
    m_render_passes.emplace_back(std::make_unique<GeometryPass>(this, shadows_fbo.texture()->id()));
    m_render_passes.emplace_back(std::make_unique<NormalsPass>(this));
    m_render_passes.emplace_back(std::make_unique<LinesPass>(this));
    m_shadows_pass = std::make_unique<ShadowsPass>(this, static_cast<GeometryPass*>(m_render_passes[0].get()));
    m_debug_pass = std::make_unique<DebugPass>(this);
    
    load(AssetManager::get_from_relative("scenes/demo.bin").value().string());
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
      glViewport(0, 0, m_window->width(), m_window->height());
      glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
      glEnable(GL_DEPTH_TEST);
      // render scene before gui to make sure that imgui window always will be on top of drawn entities
      if (m_camera.get_projection_mode() == Camera::ProjectionMode::PERSPECTIVE)
        render_skybox();
      for (auto& rp : m_render_passes)
      {
        rp->tick();
      }
      //m_debug_pass->tick();
      m_ui.tick();
      main_fbo.unbind();
      m_screen_quad.tick();
      if (m_show_shadow_map)
        m_shadow_map_quad.tick();
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
    // fix to avoid camera jumps ???
    // again, cursor pos in handler can have huge offset difference at this point
    m_window->get_input_handler<CursorPositionHandler>(UserInputHandler::CURSOR_POSITION)->update_ignore_frames();
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

    ifs.read(reinterpret_cast<char*>(&m_polygon_mode), sizeof(GLint));
    ifs.read(reinterpret_cast<char*>(&m_camera), sizeof(Camera));
    // camera is always unfreezed
    m_camera.unfreeze();
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
      if (drawable->is_selected())
      {
        m_selected_objects.push_back(drawable.get());
      }
      // calculate object center + bbox
      drawable->update();
    }

    calculate_scene_bbox();
    // setup directional light
    // center is in world space
    const glm::vec3 bbox_center = m_bbox.center();
    const glm::vec3 dir_light_position = glm::vec3(bbox_center.x + 0.12, bbox_center.y + 0.33, bbox_center.z + 0.7);
    const glm::vec3 dir_light_target = bbox_center;
    constexpr float val = 6.f;
    m_directional_light.proj_matrix = glm::ortho<float>(-val, val, -val, val, -10.f, 10.f);
    m_directional_light.view_matrix = glm::lookAt(dir_light_position, dir_light_target, glm::vec3(0, 1, 0));
    m_directional_light.direction = glm::normalize(dir_light_target - dir_light_position);

    // update render passes in accordance with the new scene
    for (auto& render_pass : m_render_passes)
    {
      render_pass->update();
    }
    update_shadow_map();
    
    // fix to avoid camera jumps ???
    // again, cursor pos in handler can have huge offset difference at this point
    m_window->get_input_handler<CursorPositionHandler>(UserInputHandler::CURSOR_POSITION)->update_ignore_frames();
  }

  void SceneRenderer::clear()
  {
    while (!m_drawables.empty())
    {
      remove_object(m_drawables.front().get());
    }
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
      if (name == "shadowMap")
      {
        continue;
      }
      fbo.bind();
      // resize texture and render buffer
      fbo.texture()->resize(width, height, fbo.texture()->internal_fmt(), fbo.texture()->format(), fbo.texture()->pixel_data_type());
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
        if (m_camera.freezed() && !m_ui.get_component("SceneInfo")->is_visible())
        {
          glfwSetInputMode(m_window->gl_window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
          m_camera.unfreeze();
        }
        else
        {
          glfwSetInputMode(m_window->gl_window(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
          m_camera.freeze();
        }
      }
      else if (key == Key::O)
      {
        m_show_shadow_map = !m_show_shadow_map;
      }
      else if (key == Key::P)
      {
        Camera::ProjectionMode current_mode = m_camera.get_projection_mode();
        m_camera.set_projection_mode(current_mode == Camera::ProjectionMode::ORTHOGRAPHIC ? 
          Camera::ProjectionMode::PERSPECTIVE : Camera::ProjectionMode::ORTHOGRAPHIC);
      }
      else if (key == Key::L && !m_selected_objects.empty())
      {
        // drop selected object on object below
        Object3D* selected_obj = m_selected_objects.front();
        //static constexpr int lower_bbox_points_indices[] = {0, 1, 4, 5};
        // TODO: iterate only over bottom bbox points
        const auto& points = selected_obj->bbox().points();
        float distance = INFINITY;
        m_debug_pass->clear();
        for (const Vertex& v : points)
        {
          Ray ray(selected_obj->model_matrix() * glm::vec4(v.position, 1), glm::vec3(0, -1, 0));
          for (const auto& drawable : m_drawables)
          {
            if (drawable.get() == selected_obj)
              continue;
            BoundingBox bbox_world;
            bbox_world.init(drawable->model_matrix() * glm::vec4(drawable->bbox().min(), 1)
              , drawable->model_matrix() * glm::vec4(drawable->bbox().max(), 1));
            if (auto hit = ray.intersect_aabb(bbox_world))
            {
              m_debug_pass->add_poly(Polyline({ ray.get_origin(), hit->position }));
              //Logger::info("hit {} distance {}", hit->position, hit->distance);
              distance = std::min(hit->distance, distance);
            }
          }
        }
        if (distance != INFINITY)
        {
          m_debug_pass->update();
          distance = distance - selected_obj->translation().y;
          auto& mm = selected_obj->model_matrix();
          // change Y translation
          mm[3][1] = -distance;
          update_shadow_map();
        }
      }
      else if (key == Key::BACKSPACE && !m_selected_objects.empty())
      {
        assert(m_selected_objects.size() == 1);
        Object3D* selected_obj = m_selected_objects.front();
        remove_object(m_selected_objects.front());
        m_selected_objects.pop_back();
      }
    }
  }

  void SceneRenderer::change_polygon_mode(int new_mode)
  {
    m_polygon_mode = new_mode;
  }

  void SceneRenderer::handle_added_object(Object3D* obj)
  {
    ObjectChangeInfo info;
    info.is_transformation_change = true;
    // trigger function that already handles needed logic
    handle_object_change(obj, info);
  }

  void SceneRenderer::handle_object_change(Object3D* obj, const ObjectChangeInfo& info)
  {
    if (info.is_transformation_change)
    {
      calculate_scene_bbox();
      update_shadow_map();
      m_fbos.at("main").bind();
    }
  }

  void SceneRenderer::calculate_scene_bbox()
  {
    m_bbox.reset();
    for (auto& drawable : m_drawables)
    {
      if (drawable->bbox().is_empty())
        drawable->calculate_bbox();
      // in world space
      glm::vec3 min_world = drawable->model_matrix() * glm::vec4(drawable->bbox().min(), 1);
      glm::vec3 max_world = drawable->model_matrix() * glm::vec4(drawable->bbox().max(), 1);
      m_bbox.grow(min_world, max_world);
    }
  }

  void SceneRenderer::update_shadow_map()
  {
    auto& shadows_fbo = m_fbos.at("shadowMap");
    shadows_fbo.bind();
    glEnable(GL_DEPTH_TEST);
    glCullFace(GL_FRONT);
    glViewport(0, 0, SHADOWMAP_WIDTH, SHADOWMAP_HEIGHT);
    glClear(GL_DEPTH_BUFFER_BIT);
    m_shadows_pass->update();
    glCullFace(GL_BACK);
    shadows_fbo.unbind();
    glViewport(0, 0, m_window->width(), m_window->height());
  }

  void SceneRenderer::handle_file_explorer_opening()
  {
    m_camera.freeze();
    glfwSetInputMode(m_window->gl_window(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  }

  void SceneRenderer::handle_file_explorer_closing()
  {
    // if scene info panel is visible, keep camera frozen because it is frozen when you open this panel
    if (!m_ui.get_component("SceneInfo")->is_visible())
    {
      m_camera.unfreeze();
      glfwSetInputMode(m_window->gl_window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
  }

  void SceneRenderer::remove_object(Object3D* obj)
  {
    on_object_delete.notify(obj);
    auto it = std::find_if(m_drawables.begin(), m_drawables.end(), [=](const auto& drawable) { return drawable.get() == obj; });
    m_drawables.erase(it);
    for (auto& rp : m_render_passes)
    {
      rp->update();
    }
    update_shadow_map();
    m_debug_pass->update();
    calculate_scene_bbox();
    m_fbos.at("main").bind();
  }

  //void SceneRenderer::create_scene()
  //{
  //  Vertex arr[6];
  //  arr[0].position = glm::vec3(0.f, 1.f, 0.f), arr[0].color = glm::vec4(0.f, 1.f, 0.f, 1.f);
  //  arr[1].position = glm::vec3(0.f, 0.f, 0.f), arr[1].color = glm::vec4(0.f, 1.f, 0.f, 1.f);
  //  arr[2].position = glm::vec3(0.f, 0.f, 0.f), arr[2].color = glm::vec4(1.f, 0.f, 0.f, 1.f);
  //  arr[3].position = glm::vec3(1.f, 0.f, 0.f), arr[3].color = glm::vec4(1.f, 0.f, 0.f, 1.f);
  //  arr[4].position = glm::vec3(0.f, 0.f, 0.f), arr[4].color = glm::vec4(0.f, 0.f, 1.f, 1.f);
  //  arr[5].position = glm::vec3(0.f, 0.f, 1.f), arr[5].color = glm::vec4(0.f, 0.f, 1.f, 1.f);
  //  auto origin = std::make_unique<Polyline>();
  //  for (int i = 0; i < 6; i++) {
  //    origin->add(arr[i]);
  //  }
  //  m_drawables.push_back(std::move(origin));
  //
  //  auto& sun = std::make_unique<Icosahedron>();
  //  sun->light_source(true);
  //  sun->translate(glm::vec3(0.f, 0.6f, 2.f));
  //  sun->set_color(glm::vec4(1.f, 1.f, 0.f, 1.f));
  //  sun->set_is_fixed_shading(true);
  //  sun->scale(glm::vec3(0.3f));
  //  sun->subdivide_triangles(4);
  //  sun->project_points_on_sphere();
  //  m_drawables.push_back(std::move(sun));
  //  m_light_sources.insert(1);
  //
  //  auto& sphere = std::make_unique<Icosahedron>();
  //  sphere->translate(glm::vec3(2.5f, 0.5f, 2.f));
  //  sphere->set_color(glm::vec4(1.f, 0.f, 0.f, 1.f));
  //  sphere->subdivide_triangles(4);
  //  sphere->project_points_on_sphere();
  //  sphere->scale(glm::vec3(0.3f));
  //  sphere->apply_shading(Object3D::ShadingMode::SMOOTH_SHADING);
  //  m_drawables.push_back(std::move(sphere));
  //
  //  auto& c = std::make_unique<Cube>();
  //  c->translate(glm::vec3(0.25f));
  //  c->scale(glm::vec3(0.5f));
  //  c->apply_shading(Object3D::ShadingMode::FLAT_SHADING);
  //  c->get_mesh(0).set_texture(std::make_shared<Texture2D>(AssetManager::get_from_relative("textures/brick.jpg").value()), TextureType::GENERIC);
  //  m_drawables.push_back(std::move(c));
  //
  //  auto& c2 = std::make_unique<Cube>();
  //  c2->translate(glm::vec3(1.25f, 1.f, 1.f));
  //  c2->set_color(glm::vec4(0.4f, 1.f, 0.4f, 1.f));
  //  c2->apply_shading(Object3D::ShadingMode::FLAT_SHADING);
  //  c2->visible_normals(true);
  //  m_drawables.push_back(std::move(c2));
  //
  //  auto& pyr = std::make_unique<Pyramid>();
  //  pyr->translate(glm::vec3(0.75f, 0.65f, 2.25f));
  //  pyr->scale(glm::vec3(0.5f));
  //  pyr->set_color(glm::vec4(0.976f, 0.212f, 0.98f, 1.f));
  //  pyr->apply_shading(Object3D::ShadingMode::FLAT_SHADING);
  //  m_drawables.push_back(std::move(pyr));
  //
  //  auto& bc = std::make_unique<BezierCurve>(BezierCurveType::Quadratic);
  //  bc->set_start_point(Vertex());
  //  bc->set_end_point(Vertex(2.5f, 0.f, 0.f));
  //  bc->set_control_points({ Vertex(1.25f, 2.f, 0.f) });
  //  bc->set_color(glm::vec4(1.f, 0.f, 0.f, 1.f));
  //  m_drawables.push_back(std::move(bc));
  //
  //  auto& bc2 = std::make_unique<BezierCurve>(BezierCurveType::Cubic);
  //  bc2->set_start_point(Vertex());
  //  bc2->set_end_point(Vertex(0.f, 0.f, -2.5f));
  //  bc2->set_control_points({ Vertex(0.f, 2.f, -1.25f), Vertex {0.f, -2.f, -1.75} });
  //  m_drawables.push_back(std::move(bc2));
  //}

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
    m_window->get_input_handler<CursorPositionHandler>(UserInputHandler::CURSOR_POSITION)->update_current_pos(x, y);

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
