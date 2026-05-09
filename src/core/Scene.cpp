#include "Scene.hpp"
#include "WindowGLFW.hpp"
#include "Shader.hpp"
#include "opengl/VertexArrayObject.hpp"
#include "opengl/VertexBufferObject.hpp"
#include "opengl/ElementBufferObject.hpp"
#include "input/InputSystem.hpp"
#include "Camera.hpp"
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
#include "ge/Object3D.hpp"
#include "ObjectsRegistry.hpp"
#include "TextureManager.hpp"
#include "utils/Utils.hpp"
#include "AssetManager.hpp"
#include "RotationController.hpp"
#include "EntityManager.hpp"
#include "SceneGraphManager.hpp"
#include "ObjectChangeInfo.hpp"
#include "Globals.hpp"

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
  const std::filesystem::path SKYBOX_TEXTURES_FOLDER = fury::AssetManager::get_assets_folder() / "textures" / "skybox";
  // clang-format off
  std::array<std::filesystem::path, 6> skybox_faces =
  {
    SKYBOX_TEXTURES_FOLDER / "right.jpg",
    SKYBOX_TEXTURES_FOLDER / "left.jpg",
    SKYBOX_TEXTURES_FOLDER / "top.jpg",
    SKYBOX_TEXTURES_FOLDER / "bottom.jpg",
    SKYBOX_TEXTURES_FOLDER / "front.jpg",
    SKYBOX_TEXTURES_FOLDER / "back.jpg"
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
  // clang-format on
  constexpr std::array shadow_map_data = init_shadow_map_data();
} // namespace

namespace fury
{
  void Scene::init(WindowGLFW* window)
  {
    m_window = window;
    const int w = window->width();
    const int h = window->height();
    m_ui.init(this);
    m_cam_controller.init(&m_camera);
    SceneInfo* scene_info_component = m_ui.get_component<SceneInfo>("SceneInfo");
    Gizmo* gizmo_component = m_ui.get_component<Gizmo>("Gizmo");
    FileExplorer* file_explorer_component = m_ui.get_component<FileExplorer>("FileExplorer");
    file_explorer_component->on_show += new InstanceListener(this, &Scene::handle_ui_component_opening);
    file_explorer_component->on_hide += new InstanceListener(this, &Scene::handle_ui_component_closing);
    scene_info_component->on_show += new InstanceListener(this, &Scene::handle_ui_component_opening);
    scene_info_component->on_hide += new InstanceListener(this, &Scene::handle_ui_component_closing);
    scene_info_component->on_polygon_mode_change += new InstanceListener(this, &Scene::change_polygon_mode);
    scene_info_component->msaa_button_click += new InstanceListener(this, &Scene::handle_msaa_button_toggle);
    on_new_object_added += new FunctionListener(std::function(
        [this](Object3D* obj)
        {
          ObjectChangeInfo info;
          info.object = obj;
          info.new_transform = SceneGraphManager::get_entity_node<TransformationSceneNode>(obj->get_id());
          handle_object_change(info);
          EntityManager::add_entity(obj);
        }));
    global_state::g_on_object_change +=
        new FunctionListener(std::function([this](const ObjectChangeInfo& info) { handle_object_change(info); }));

    m_camera.set_screen_size({ w, h });

    InputSystem& input_system = InputSystem::instance();
    input_system.on_keyboard_button_clicked += new InstanceListener(this, &Scene::handle_keyboard_button_click);
    input_system.on_mouse_button_clicked += new InstanceListener(this, &Scene::handle_mouse_click);
    m_window->on_window_size_change += new InstanceListener(this, &Scene::handle_window_size_change);

    auto& main_scene_fbo = m_fbos["main"];
    main_scene_fbo.bind();
    main_scene_fbo.attach_texture(Texture2D(w, h, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE));
    RenderBufferCreateInfo info = { w, h };
    main_scene_fbo.attach_renderbuffer(info);
    main_scene_fbo.unbind();

    auto& main_ms = m_fbos["mainMS"];
    main_ms.bind();
    main_ms.attach_texture_ms(Texture2DMS(w, h, GL_RGB));
    RenderBufferCreateInfo info2 = { w, h };
    info2.is_multisampled = true;
    main_ms.attach_renderbuffer(info2);
    main_ms.unbind();

    auto& shadows_fbo = m_fbos["shadowMap"];
    shadows_fbo.bind();
    shadows_fbo.attach_texture(
        Texture2D(SHADOWMAP_WIDTH, SHADOWMAP_HEIGHT, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT), false);
    shadows_fbo.texture()->bind();
    // anything that is out of depth map is not in shadow
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
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
    m_fps_limiter.set_limit(glfwGetVideoMode(glfwGetPrimaryMonitor())->refreshRate);

    UniformBuffer& ubo = PipelineUBOManager::get("cameraData");
    ubo.bind();
    ubo.resize(sizeof(glm::mat4) * 2);
    ubo.set_binding_point(0);
    ubo.unbind();

    SelectionWheelConfig cfg;
    cfg.items_count = 6;
    cfg.inner_circle_radius_px = 200;
    cfg.outer_circle_radius_px = 400;
    cfg.slots_spacing_deg = 0;
    m_selection_wheel.init(m_window->width(), m_window->height(), cfg);

    // TODO: remove later. just for testing purposes now
    for (int i = 0; i < cfg.items_count; i++)
    {
      if (i % 2 == 0)
      {
        m_selection_wheel.get_slot(i)->icon =
            TextureManager::get(AssetManager::get_absolute_from_relative("textures/brick.jpg").value()).get();
      }
    }

    m_render_passes.emplace_back(std::make_unique<GeometryPass>(this, shadows_fbo.texture()->id()));
    m_render_passes.emplace_back(std::make_unique<NormalsPass>(this));
    m_render_passes.emplace_back(std::make_unique<SelectionWheelPass>(this, &m_selection_wheel));
    m_render_passes.emplace_back(std::make_unique<InfiniteGridPass>(this));
    m_shadows_pass = std::make_unique<ShadowsPass>(this, static_cast<GeometryPass*>(m_render_passes[0].get()));

    // load(AssetManager::get_from_relative("scenes/demo.bin").value().string());
    create_default_scene();
  }

  Scene::~Scene()
  {
  }

  void Scene::render()
  {
    using namespace std::chrono;
    using namespace std::chrono_literals;
    const auto& main_fbo = m_fbos.at("main");
    const auto& main_fbo_ms = m_fbos.at("mainMS");
    GLFWwindow* gl_window = m_window->gl_window();
    ImGuiIO& io = ImGui::GetIO();
    steady_clock::time_point prev_frame_time;
    steady_clock::time_point fps_timer = steady_clock::now();
    int frame_count_per_sec = 0;
    while (!glfwWindowShouldClose(gl_window))
    {
      steady_clock::time_point frame_time_start = steady_clock::now();
      if ((steady_clock::now() - fps_timer) >= 1s)
      {
        m_render_info.fps = frame_count_per_sec;
        frame_count_per_sec = 0;
        fps_timer += 1s;
      }
      const float dt = m_render_info.frame_time;
      glfwPollEvents();
      tick(dt);
      glPolygonMode(GL_FRONT_AND_BACK, m_polygon_mode);

      const int w = m_window->width();
      const int h = m_window->height();
      // render to a custom framebuffer
      if (m_MSAA_enabled)
      {
        main_fbo_ms.bind();
      }
      else
      {
        main_fbo.bind();
      }
      glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
      glEnable(GL_DEPTH_TEST);
      // render scene before gui to make sure that imgui window always will be on top of drawn entities
      if (m_camera.get_projection_mode() == Camera::ProjectionMode::PERSPECTIVE)
      {
        render_skybox();
      }
      for (auto& rp : m_render_passes)
      {
        rp->tick(dt);
      }
      DebugPass::instance().tick(dt);
      m_ui.tick(dt);

      if (m_MSAA_enabled)
      {
        // copy pixels from MSAA FBO to FBO that's texture is being rendered
        glBindFramebuffer(GL_READ_FRAMEBUFFER, main_fbo_ms.id());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, main_fbo.id());
        glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
      }
      glBindFramebuffer(GL_FRAMEBUFFER, 0);

      m_screen_quad.tick(dt);
      if (m_show_shadow_map)
      {
        m_shadow_map_quad.tick(dt);
      }
      m_fps_limiter.wait();
      glfwSwapBuffers(gl_window);
      frame_count_per_sec++;
      m_render_info.frame_time = std::chrono::duration<float>(frame_time_start - prev_frame_time).count();
      prev_frame_time = frame_time_start;
      SceneGraphManager::clear_dirty_nodes();
    }
  }

  void Scene::save(const std::string& file) const
  {
    std::ofstream ofs(file, std::ios_base::binary);
    if (!ofs.is_open())
    {
      Logger::error("Scene::save failed to open file {}.", file);
      return;
    }
    serializer::prepare_for_serialization();
    Serializer<Scene>::write(ofs, this);
    // TODO: rewrite
    SceneGraphManager::write(ofs);
    ofs.close();
    // fix to avoid camera jumps ???
    // again, cursor pos in handler can have huge offset difference at this point
    // InputSystem::instance().cursor_movement_ignore_first_frames();
  }

  void Scene::load(const std::string& file)
  {
    std::ifstream ifs(file, std::ios_base::binary);
    if (!ifs.is_open())
    {
      Logger::error("Load failed to open file {}.", file);
      return;
    }
    cleanup();
    SceneGraphManager::clear();
    EntityManager::clear();
    serializer::prepare_for_serialization();
    Serializer<Scene>::read(ifs, this);
    SceneGraphManager::read(ifs);
    ifs.close();
    prepare_scene_for_rendering();
  }

  void Scene::clear()
  {
    while (!m_drawables.empty())
    {
      remove_object(m_drawables.front().get());
    }
  }

  void Scene::render_skybox()
  {
    glDepthFunc(GL_LEQUAL);
    Shader* shader = &ShaderStorage::get(ShaderStorage::ShaderType::SKYBOX);
    shader->bind();
    glActiveTexture(GL_TEXTURE0);
    m_skybox.render();
    shader->unbind();
    glDepthFunc(GL_LESS);
  }

  void Scene::handle_mouse_click(InputCode input_code, int x, int y)
  {
    if (input_code == InputCode::FURY_MOUSE_BUTTON_LEFT)
    {
      if (m_selection_wheel.is_visible())
      {
        return;
      }
      y = static_cast<int>(m_camera.get_screen_size().y) - y;
      Ray ray = m_camera.cast_ray(x, y);
      std::pair<Object3D*, float> hit_info = { nullptr, INFINITY };
#if DEBUG_RAY
      std::vector<Polyline> rays;
#endif
      for (const auto& d : m_drawables)
      {
        TransformationSceneNode* node = SceneGraphManager::get_entity_node<TransformationSceneNode>(d->get_id());
        const glm::mat4 inv_model_mat = glm::inverse(node->get_world_mat());
        Ray ray_local =
            Ray(inv_model_mat * glm::vec4(ray.get_origin(), 1), inv_model_mat * glm::vec4(ray.get_direction(), 0));
        if (auto hit = ray_local.intersect_object3d(d.get()))
        {
          const glm::vec3 ray_hit_pos_world = node->get_world_mat() * glm::vec4(hit->position, 1);
#if DEBUG_RAY
          Polyline poly;
          poly.add(ray.get_origin());
          poly.add(ray_hit_pos_world);
          // auto dist = glm::distance(ray.get_origin(), glm::vec3(d->model_matrix() * glm::vec4(hit->position, 1)));
          // std::cout << "dist " << d->name() << " " << dist << '\n';
          rays.push_back(poly);
#endif
          // calculate distance in world space and find closest object. my guess is that we cannot use rayhit->distance
          // here, because it's in local space, therefore, if ray instersects multiple objects, the object that is
          // further may have less distance value than the object that is closer.
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

  void Scene::handle_window_size_change(int width, int height)
  {
    // to avoid crash when window is completely minimized
    if (width == 0 || height == 0)
    {
      return;
    }
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
      if (auto& tex = fbo.texture())
      {
        tex->resize(width, height, fbo.texture()->internal_fmt(), fbo.texture()->format(),
                    fbo.texture()->pixel_data_type());
      }
      if (auto& texMS = fbo.texture_ms())
      {
        // multisample texture is immutable, so replace old one with the new one
        fbo.attach_texture_ms(Texture2DMS(width, height, texMS->get_format(), texMS->get_samples()));
      }
      RenderBufferCreateInfo info = fbo.get_info();
      info.w = width;
      info.h = height;
      fbo.attach_renderbuffer(info);
      fbo.unbind();
    }
    glViewport(0, 0, width, height);
  }

  void Scene::handle_keyboard_button_click(InputCode input_code)
  {
    const InputSystem& input_system = InputSystem::instance();
    if (input_code == InputCode::FURY_KEY_ESC)
    {
      assert(m_selected_objects.empty() || m_selected_objects.size() == 1);
      for (Object3D* obj : m_selected_objects)
      {
        m_selected_objects.pop_back();
        obj->select(false);
      }
    }
    else if (input_code == InputCode::FURY_KEY_O)
    {
      m_show_shadow_map = !m_show_shadow_map;
    }
    else if (input_code == InputCode::FURY_KEY_P)
    {
      Camera::ProjectionMode current_mode = m_camera.get_projection_mode();
      m_camera.set_projection_mode(current_mode == Camera::ProjectionMode::ORTHOGRAPHIC
                                       ? Camera::ProjectionMode::PERSPECTIVE
                                       : Camera::ProjectionMode::ORTHOGRAPHIC);
    }
    else if (input_code == InputCode::FURY_KEY_L && !m_selected_objects.empty())
    {
      // drop selected object on object below
      Object3D* selected_obj = m_selected_objects.front();
      // static constexpr int lower_bbox_points_indices[] = {0, 1, 4, 5};
      //  TODO: iterate only over bottom bbox points
      const auto points = selected_obj->get_bbox().get_points();
      TransformationSceneNode* selected_transform =
          SceneGraphManager::get_entity_node<TransformationSceneNode>(selected_obj->get_id());
      float distance = INFINITY;
      DebugPass::instance().clear();
      for (const glm::vec3& p : points)
      {
        Ray ray(selected_transform->get_world_mat() * glm::vec4(p, 1), glm::vec3(0, -1, 0));
        for (const auto& drawable : m_drawables)
        {
          if (drawable.get() == selected_obj)
          {
            continue;
          }
          BoundingBox bbox_world;
          TransformationSceneNode* drawable_transform =
              SceneGraphManager::get_entity_node<TransformationSceneNode>(drawable->get_id());
          bbox_world.init(drawable_transform->get_world_mat() * glm::vec4(drawable->get_bbox().min(), 1),
                          drawable_transform->get_world_mat() * glm::vec4(drawable->get_bbox().max(), 1));
          if (auto hit = ray.intersect_aabb(bbox_world))
          {
            DebugPass::instance().add_line(ray.get_origin(), hit->position);
            // Logger::info("hit {} distance {}", hit->position, hit->distance);
            distance = std::min(hit->distance, distance);
          }
        }
      }
      if (distance != INFINITY)
      {
        glm::vec3 old = selected_transform->get_translation();
        distance = distance - old.y;
        selected_transform->set_translation(glm::vec3(old.x, -distance, old.z));
        update_shadow_map();
      }
    }
    else if (input_code == InputCode::FURY_KEY_BACKSPACE && !m_selected_objects.empty())
    {
      assert(m_selected_objects.size() == 1);
      Object3D* selected_obj = m_selected_objects.front();
      remove_object(m_selected_objects.front());
      m_selected_objects.pop_back();
    }
    else if (input_code == InputCode::FURY_KEY_GRAVE_ACCENT)
    {
      SceneInfo* scene_info = m_ui.get_component<SceneInfo>("SceneInfo");
      scene_info->is_visible() ? scene_info->hide() : scene_info->show();
    }
    else if (input_code == InputCode::FURY_KEY_Q)
    {
      const bool will_be_visible = !m_selection_wheel.is_visible();
      m_selection_wheel.select(-1);
      m_selection_wheel.set_is_visible(will_be_visible);
      if (will_be_visible)
      {
        handle_ui_component_opening();
      }
      else
      {
        handle_ui_component_closing();
      }
    }
    else if (input_code == InputCode::FURY_KEY_M)
    {
      Frustum fr = m_camera.get_frustum();
      auto& dbg = DebugPass::instance();
      for (const auto& [p1, p2] : fr.debug_lines)
      {
        dbg.add_line(p1, p2, glm::vec4(0, 1, 0, 1));
      }
    }
  }

  void Scene::change_polygon_mode(int new_mode)
  {
    m_polygon_mode = new_mode;
  }

  void Scene::handle_object_change(const ObjectChangeInfo& info)
  {
    if (info.new_transform)
    {
      calculate_scene_bbox();
      update_shadow_map();
      if (m_MSAA_enabled)
      {
        m_fbos.at("mainMS").bind();
      }
      else
      {
        m_fbos.at("main").bind();
      }
    }
  }

  void Scene::calculate_scene_bbox()
  {
    m_bbox.reset();
    for (auto& drawable : m_drawables)
    {
      if (drawable->get_bbox().is_empty())
      {
        drawable->calculate_bbox();
      }
      // in world space
      auto node = SceneGraphManager::get_entity_node<TransformationSceneNode>(drawable->get_id());
      node->update();
      const glm::vec3 min_world = node->get_world_mat() * glm::vec4(drawable->get_bbox().min(), 1);
      const glm::vec3 max_world = node->get_world_mat() * glm::vec4(drawable->get_bbox().max(), 1);
      const glm::vec3 true_min = glm::min(min_world, max_world);
      const glm::vec3 true_max = glm::max(min_world, max_world);
      m_bbox.grow(true_min, true_max);
    }
  }

  void Scene::update_shadow_map()
  {
    auto& shadows_fbo = m_fbos.at("shadowMap");
    shadows_fbo.bind();
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    // glEnable(GL_POLYGON_OFFSET_FILL);
    // glPolygonOffset(1, 1);
    // TODO: gap between coplanar planes ...
    // glCullFace(GL_FRONT);
    glViewport(0, 0, SHADOWMAP_WIDTH, SHADOWMAP_HEIGHT);
    glClear(GL_DEPTH_BUFFER_BIT);
    m_shadows_pass->update();
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    // glDisable(GL_POLYGON_OFFSET_FILL);
    shadows_fbo.unbind();
    glViewport(0, 0, m_window->width(), m_window->height());
  }

  void Scene::handle_ui_component_opening()
  {
    m_camera.freeze();
    glfwSetInputMode(m_window->gl_window(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    InputSystem::instance().push_context("UI");
  }

  void Scene::handle_ui_component_closing()
  {
    InputSystem::instance().pop_context();
    if (InputSystem::instance().get_active_input_context()->name == "FreeCam")
    {
      m_camera.unfreeze();
      glfwSetInputMode(m_window->gl_window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
  }

  void Scene::handle_msaa_button_toggle(bool enabled)
  {
    m_MSAA_enabled = enabled;
  }

  void Scene::remove_object(Object3D* obj)
  {
    on_object_deleted.notify(obj);
    auto it = std::find_if(m_drawables.begin(), m_drawables.end(),
                           [=](const auto& drawable) { return drawable.get() == obj; });
    m_drawables.erase(it);
    for (auto& rp : m_render_passes)
    {
      rp->update();
    }
    update_shadow_map();
    DebugPass::instance().update();
    calculate_scene_bbox();
    if (m_MSAA_enabled)
    {
      m_fbos.at("mainMS").bind();
    }
    else
    {
      m_fbos.at("main").bind();
    }
  }

  void Scene::cleanup()
  {
    // cleanup current scene
    m_drawables.clear();
    m_selected_objects.clear();
    m_lights.clear();
    m_controllers.clear();
    m_ui.get_component<SceneInfo>("SceneInfo")->hide();
  }

  void Scene::create_default_lights()
  {
    Light& dir_light = m_lights.emplace_back();
    dir_light.enable();
    setup_directional_light(&dir_light);

    // spot light from camera position
    LightDescription desc;
    desc.type = LightType::SPOT;
    desc.position = glm::vec4(m_camera.get_position(), 1);
    desc.dir = glm::vec4(m_camera.get_target(), 1);
    Light& spot_light = m_lights.emplace_back(desc);
    TransformationSceneNode* transform = spot_light.attach_node<TransformationSceneNode>();
    transform->set_parent(SceneGraphManager::get_entity_node<TransformationSceneNode>(m_camera.get_id()));
  }

  void Scene::setup_directional_light(Light* light)
  {
    // center is in world space
    const glm::vec3 bbox_center = m_bbox.center();
    const glm::vec3 dir_light_position = glm::vec3(bbox_center.x + 0.12, bbox_center.y + 0.33, bbox_center.z + 0.7);
    const glm::vec3 dir_light_target = bbox_center;
    constexpr static float cube_bound = 6.f;
    LightDescription desc;
    desc.type = LightType::DIRECTIONAL;
    desc.dir = glm::vec4(glm::normalize(dir_light_target - dir_light_position), 1);
    desc.position = glm::vec4(dir_light_position + cube_bound * -glm::vec3(desc.dir), 1);
    const glm::mat4 proj_mat = glm::ortho<float>(-cube_bound, cube_bound, -cube_bound, cube_bound, -10.f, 10.f);
    const glm::mat4 view_mat = glm::lookAt(dir_light_position, dir_light_target, glm::vec3(0, 1, 0));
    desc.shadow_matrix = proj_mat * view_mat;
    TransformationSceneNode* transform = light->attach_node<TransformationSceneNode>();
    transform->set_translation(desc.position);
    light->set_description(desc);
  }

  std::vector<const Light*> Scene::get_active_lights() const
  {
    std::vector<const Light*> active_lights;
    active_lights.reserve(m_lights.size());
    for (const Light& l : m_lights)
    {
      if (l.is_enabled())
      {
        active_lights.push_back(&l);
      }
    }
    return active_lights;
  }

  std::vector<Light*> Scene::get_lights(LightType type)
  {
    std::vector<Light*> lights;
    lights.reserve(m_lights.size());
    for (int i = 0; i < static_cast<int>(m_lights.size()); i++)
    {
      if (m_lights[i].get_type() == type)
      {
        lights.push_back(&m_lights[i]);
      }
    }
    return lights;
  }

  void Scene::create_default_scene()
  {
    cleanup();
    m_camera.attach_node<TransformationSceneNode>();
    m_camera.set_position(glm::vec3(-4.f, 2.f, 3.f));
    m_camera.look_at(glm::vec3(2.f, 0.5f, 0.5f));
    EntityManager::add_entity(&m_camera);

    Vertex arr[6];
    arr[0].position = glm::vec3(0.f, 1.f, 0.f), arr[0].color = glm::vec4(0.f, 1.f, 0.f, 1.f);
    arr[1].position = glm::vec3(0.f, 0.f, 0.f), arr[1].color = glm::vec4(0.f, 1.f, 0.f, 1.f);
    arr[2].position = glm::vec3(0.f, 0.f, 0.f), arr[2].color = glm::vec4(1.f, 0.f, 0.f, 1.f);
    arr[3].position = glm::vec3(1.f, 0.f, 0.f), arr[3].color = glm::vec4(1.f, 0.f, 0.f, 1.f);
    arr[4].position = glm::vec3(0.f, 0.f, 0.f), arr[4].color = glm::vec4(0.f, 0.f, 1.f, 1.f);
    arr[5].position = glm::vec3(0.f, 0.f, 1.f), arr[5].color = glm::vec4(0.f, 0.f, 1.f, 1.f);
    auto origin = std::make_unique<Polyline>();
    for (int i = 0; i < 6; i++)
    {
      origin->add(arr[i]);
    }
    m_drawables.push_back(std::move(origin));

    auto sun = m_drawables.emplace_back(std::make_unique<Icosahedron>())->cast_to<Icosahedron>();
    TransformationSceneNode* transform = sun->attach_node<TransformationSceneNode>();
    transform->set_translation(glm::vec3(0.f, 0.6f, 2.f));
    transform->set_scale(glm::vec3(0.3f));
    sun->set_color(glm::vec4(1.f, 1.f, 0.f, 1.f));
    sun->set_is_fixed_shading(true);
    sun->subdivide_triangles(4);
    sun->project_points_on_sphere();

    auto sphere = m_drawables.emplace_back(std::make_unique<Icosahedron>())->cast_to<Icosahedron>();
    transform = sphere->attach_node<TransformationSceneNode>();
    transform->set_translation(glm::vec3(2.5f, 0.5f, 2.f));
    transform->set_scale(glm::vec3(0.3f));
    sphere->set_color(glm::vec4(1.f, 0.f, 0.f, 1.f));
    sphere->subdivide_triangles(4);
    sphere->project_points_on_sphere();
    sphere->apply_shading(Object3D::ShadingMode::SMOOTH_SHADING);

    auto c = m_drawables.emplace_back(std::make_unique<Cube>())->cast_to<Cube>();
    transform = c->attach_node<TransformationSceneNode>();
    transform->set_translation(glm::vec3(0.25f));
    transform->set_scale(glm::vec3(0.5f));
    c->apply_shading(Object3D::ShadingMode::FLAT_SHADING);
    c->get_mesh(0).set_texture(
        TextureManager::get(AssetManager::get_absolute_from_relative("textures/brick.jpg").value()),
        TextureType::GENERIC);

    c = m_drawables.emplace_back(std::make_unique<Cube>())->cast_to<Cube>();
    transform = c->attach_node<TransformationSceneNode>();
    transform->set_translation(glm::vec3(1.25f, 1.f, 1.f));
    c->set_color(glm::vec4(0.4f, 1.f, 0.4f, 1.f));
    c->apply_shading(Object3D::ShadingMode::FLAT_SHADING);
    c->visible_normals(true);

    auto pyr = m_drawables.emplace_back(std::make_unique<Pyramid>())->cast_to<Pyramid>();
    transform = pyr->attach_node<TransformationSceneNode>();
    transform->set_translation(glm::vec3(0.75f, 0.65f, 2.25f));
    transform->set_scale(glm::vec3(0.5f));
    pyr->set_color(glm::vec4(0.976f, 0.212f, 0.98f, 1.f));
    pyr->apply_shading(Object3D::ShadingMode::FLAT_SHADING);

    auto bc =
        m_drawables.emplace_back(std::make_unique<BezierCurve>(BezierCurveType::Quadratic))->cast_to<BezierCurve>();
    bc->set_start_point(Vertex());
    bc->set_end_point(Vertex(2.5f, 0.f, 0.f));
    bc->set_control_points({ Vertex(1.25f, 2.f, 0.f) });
    bc->set_color(glm::vec4(1.f, 0.f, 0.f, 1.f));

    bc = m_drawables.emplace_back(std::make_unique<BezierCurve>(BezierCurveType::Cubic))->cast_to<BezierCurve>();
    bc->set_start_point(Vertex());
    bc->set_end_point(Vertex(0.f, 0.f, -2.5f));
    bc->set_control_points({ Vertex(0.f, 2.f, -1.25f), Vertex { 0.f, -2.f, -1.75 } });

    for (auto& drawable : m_drawables)
    {
      if (!SceneGraphManager::get_entity_node<TransformationSceneNode>(drawable->get_id()))
      {
        drawable->attach_node<TransformationSceneNode>();
      }
      EntityManager::add_entity(drawable.get());
    }

    auto& rotation_controller =
        m_controllers.emplace_back(new RotationController(glm::vec3(1, 0, 0), glm::radians(90.f)));
    rotation_controller->set_entity(pyr);
    prepare_scene_for_rendering();
  }

  void Scene::prepare_scene_for_rendering()
  {
    for (auto& drawable : m_drawables)
    {
      if (drawable->is_selected())
      {
        m_selected_objects.push_back(drawable.get());
      }
      // calculate object center + bbox
      drawable->update();
    }

    calculate_scene_bbox();
    if (m_lights.empty())
    {
      create_default_lights();
      for (Light& light : m_lights)
      {
        EntityManager::add_entity(&light);
      }
    }

    for (auto& render_pass : m_render_passes)
    {
      render_pass->update();
    }
    update_shadow_map();

    // fix to avoid camera jumps ???
    // again, cursor pos in handler can have huge offset difference at this point
    // InputSystem::instance().cursor_movement_ignore_first_frames();
  }

  void Scene::select_object(Object3D* obj, bool click_from_menu_item)
  {
    // do not select object that is behind imgui's menu, but select it when we are clicking on obj name inside menu
    // itself
    ImGuiIO& io = ImGui::GetIO();
    if (!click_from_menu_item && io.WantCaptureMouse)
    {
      return;
    }
    if (click_from_menu_item)
    {
      // look at selected object

      TransformationSceneNode* node = SceneGraphManager::get_entity_node<TransformationSceneNode>(obj->get_id());
      m_camera.look_at(obj->center() + glm::vec3(node->get_world_mat()[3]));
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

  void Scene::tick(float dt)
  {
    for (const auto& c : m_controllers)
    {
      c->tick(dt);
    }
    m_cam_controller.tick(dt);

    for (SceneNode* node : SceneGraphManager::get_dirty_nodes())
    {
      node->update();
      if (Entity* owner = node->get_owner(); owner->is_a(Object3D::get_static_type_id()))
      {
        Object3D* obj = static_cast<Object3D*>(node->get_owner());
        ObjectChangeInfo change_info;
        change_info.object = obj;
        change_info.new_transform = static_cast<TransformationSceneNode*>(node);
        global_state::g_on_object_change.notify(change_info);
      }
    }

    UniformBuffer& ubo = PipelineUBOManager::get("cameraData");
    ubo.bind();
    ubo.set_data(&m_camera.get_view_matrix(), sizeof(glm::mat4), 0);
    ubo.set_data(&m_camera.get_projection_matrix(), sizeof(glm::mat4), sizeof(glm::mat4));
    ubo.unbind();
  }
} // namespace fury
