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
#include "Logger.hpp"
#include "Skybox.hpp"
#include "ObjectsRegistry.hpp"
#include "utils/Utils.hpp"
#include "AssetManager.hpp"
#include "ui/SceneInfoWindow.hpp"
#include "ui/Gizmo.hpp"
#include "ui/MenuBar.hpp"
#include "ui/ItemSelectionWheel.hpp"
#include "ui/FileExplorer.hpp"
#include "RenderPass.hpp"
#include "GeometryFactory.hpp"
#include "SceneGraph.hpp"

#include "ecs/components/DrawableComponent.hpp"
#include "ecs/components/LightComponent.hpp"
#include "ecs/components/TransformComponent.hpp"
#include "ecs/components/ObjectParametersComponent.hpp"
#include "ecs/components/RotationComponent.hpp"
#include "ecs/components/DirtyEntityTagComponent.hpp"

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
  const std::filesystem::path SKYBOX_TEXTURES_FOLDER = fury::utils::get_assets_folder() / "textures" / "skybox";
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
  template<typename T>
  [[nodiscard]] static std::vector<std::pair<AssetId, Ref<T>>> read_asset_manager_data_helper(std::ifstream& ifs)
  {
    // Keep assets alive until some AssetHandle will reference it
    std::vector<std::pair<AssetId, std::shared_ptr<T>>> assets;
    uint32_t num_assets;
    BinarySerializer::read(ifs, &num_assets);
    for (uint32_t i = 0; i < num_assets; i++)
    {
      AssetId id;
      BinarySerializer::read(ifs, &id);
      auto sp = std::make_shared<T>();
      BinarySerializer::read(ifs, sp.get());
      assets.emplace_back(id, sp);
      AssetManager<T>::store(id, sp);
    }
    return assets;
  }

  template<typename T>
  static void write_asset_manager_data_helper(std::ofstream& ofs)
  {
    std::vector<std::pair<AssetId, std::shared_ptr<T>>> valid_assets;
    const auto& assets = AssetManager<T>::get_assets();
    valid_assets.reserve(assets.size());
    for (const auto& [id, asset] : assets)
    {
      if (auto sp = asset.lock())
      {
        valid_assets.emplace_back(id, sp);
      }
    }
    const uint32_t num_assets = static_cast<uint32_t>(valid_assets.size());
    BinarySerializer::write(ofs, &num_assets);
    for (const auto& [id, asset] : valid_assets)
    {
      BinarySerializer::write(ofs, &id);
      BinarySerializer::write(ofs, asset.get());
    }
  }

  Scene::Scene()
  {
  }

  void Scene::init(WindowGLFW* window)
  {
    m_window = window;
    const int w = window->width();
    const int h = window->height();
    m_cam_controller.init(&m_camera);
    init_ui_components();
    m_camera.set_screen_size({ w, h });

    SceneInfoWindow& scene_info_window = m_ui.get_scene_info_window();
    scene_info_window.use_msaa_button_toggled += new FunctionListener(
        std::function([scene = this](bool use_msaa) { scene->m_scene_options.use_msaa = use_msaa; }));
    scene_info_window.fill_polygons_button_toggled += new FunctionListener(std::function(
        [scene = this](bool fill_polygons)
        {
          scene->m_scene_options.fill_polygons = fill_polygons;
          glPolygonMode(GL_FRONT_AND_BACK, fill_polygons ? GL_FILL : GL_LINE);
        }));
    scene_info_window.show_grid_button_toggled += new FunctionListener(
        std::function([scene = this](bool show_grid) { scene->m_scene_options.show_grid = show_grid; }));
    scene_info_window.frustum_culling_button_toggled += new FunctionListener(
        std::function([scene = this](bool do_frustum_culling)
                      { scene->m_scene_options.is_frustum_culling_enabled = do_frustum_culling; }));
    scene_info_window.show_scene_bbox_button_toggled += new FunctionListener(std::function(
        [scene = this](bool show_scene_bbox) { scene->m_scene_options.show_scene_bbox = show_scene_bbox; }));
    scene_info_window.visible_normals_button_toggled +=
        new InstanceListener(this, &Scene::on_entity_normals_visibility_toggle);
    scene_info_window.visible_bbox_button_toggled +=
        new InstanceListener(this, &Scene::on_entity_bbox_visibility_toggle);

    InputSystem& input_system = InputSystem::instance();
    input_system.on_keyboard_button_clicked += new InstanceListener(this, &Scene::handle_keyboard_button_click);
    input_system.on_mouse_button_clicked += new InstanceListener(this, &Scene::handle_mouse_click);
    m_window->window_size_changed += new InstanceListener(this, &Scene::handle_window_size_change);

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

    Cubemap cm;
    cm.set_textures(skybox_faces);
    m_skybox.set_cubemap(std::move(cm));
    m_screen_quad.init(main_scene_fbo.texture()->id());
    m_shadow_map_quad.init(shadow_map_data, shadows_fbo.texture()->id(), true);
    m_fps_limiter.set_limit(glfwGetVideoMode(glfwGetPrimaryMonitor())->refreshRate);

    m_camera_data_ubo.bind();
    m_camera_data_ubo.resize(sizeof(glm::mat4) * 2);
    m_camera_data_ubo.unbind();
    m_camera_data_ubo.set_binding_point(0);
    m_camera_data_ubo.bind_to_index();

    m_transforms_ssbo.set_binding_point(1);
    m_lights_ssbo.set_binding_point(2);

    m_render_passes = { &m_geometry_pass, &m_shadows_pass, &m_grid_pass, &m_debug_pass, &m_selection_wheel_pass };
    m_geometry_pass.set_shadow_map_texture(shadows_fbo.texture()->id());
    m_shadows_pass.set_geometry_pass_ref(&m_geometry_pass);
    for (RenderPass* rp : m_render_passes)
    {
      rp->init(this);
    }
    // load(AssetManager::get_from_relative("scenes/demo.bin").value().string());
    create_default_scene();
  }

  Scene::~Scene()
  {
  }

  void Scene::init_ui_components()
  {
    m_ui.init(this);
    SelectionWheelConfig selection_wheel_cfg {
      .items_count = 6,
      .inner_circle_radius_px = 200,
      .outer_circle_radius_px = 400,
      .slots_spacing_deg = 0,
    };
    ItemSelectionWheel& item_seleciton_wheel = m_ui.get_item_selection_wheel();
    item_seleciton_wheel.set_config(selection_wheel_cfg);
    for (int i = 0; i < selection_wheel_cfg.items_count; i++)
    {
      if (i % 2 == 0)
      {
        item_seleciton_wheel.get_slot(i)->icon = TextureManager::load(utils::get_textures_folder() / "brick.jpg");
      }
    }
    const auto& ui_components = m_ui.get_components();
    std::for_each(ui_components.begin(), ui_components.end(),
                  [scene = this](UiComponent* component)
                  {
                    component->on_show += new InstanceListener(scene, &Scene::handle_ui_component_opening);
                    component->on_hide += new InstanceListener(scene, &Scene::handle_ui_component_closing);
                  });
  }

  void Scene::process_async_results()
  {
    // Process async results from other threads
    if (!m_async_tasks.empty())
    {
      std::lock_guard lg(m_async_tasks_mutex);
      for (auto& task : m_async_tasks)
      {
        task();
      }
      m_async_tasks.clear();
    }
  }

  void Scene::render()
  {
    using namespace std::chrono;
    using namespace std::chrono_literals;
    const auto& main_fbo = m_fbos.at("main");
    const auto& main_fbo_ms = m_fbos.at("mainMS");
    GLFWwindow* gl_window = m_window->gl_window();
    steady_clock::time_point prev_frame_time;
    steady_clock::time_point fps_timer = steady_clock::now();
    int frame_num_per_sec = 0;
    while (!glfwWindowShouldClose(gl_window))
    {
      process_async_results();
      steady_clock::time_point frame_time_start = steady_clock::now();
      if ((steady_clock::now() - fps_timer) >= 1s)
      {
        m_fps = frame_num_per_sec;
        frame_num_per_sec = 0;
        fps_timer += 1s;
      }
      const float dt = m_frame_info.frame_time;
      glfwPollEvents();
      tick(dt);

      const int w = m_window->width();
      const int h = m_window->height();
      // render to a custom framebuffer
      if (m_scene_options.use_msaa)
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

      m_transforms_ssbo.bind_to_index();
      for (auto rp : m_render_passes)
      {
        rp->tick(dt);
      }
      m_frame_info.num_objects_culled = m_geometry_pass.get_num_culled_objects();
      m_ui.tick(dt);

      if (m_scene_options.use_msaa)
      {
        // copy pixels from MSAA FBO to FBO that's texture is being rendered
        glBindFramebuffer(GL_READ_FRAMEBUFFER, main_fbo_ms.id());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, main_fbo.id());
        glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
      }
      glBindFramebuffer(GL_FRAMEBUFFER, 0);

      // set GL_FILL mode because next we are rendering screen textures
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      m_screen_quad.tick(dt);
      if (m_scene_options.show_shadow_map)
      {
        m_shadow_map_quad.tick(dt);
      }
      glPolygonMode(GL_FRONT_AND_BACK, m_scene_options.fill_polygons ? GL_FILL : GL_LINE);
      m_fps_limiter.wait();
      glfwSwapBuffers(gl_window);
      frame_num_per_sec++;
      m_frame_info.frame_time = std::chrono::duration<float>(frame_time_start - prev_frame_time).count();
      prev_frame_time = frame_time_start;
    }
  }

  void Scene::save(const std::filesystem::path& file) const
  {
    std::ofstream ofs(file, std::ios_base::binary);
    if (!ofs.is_open())
    {
      Logger::error("Scene::save failed to open file {}.", file);
      return;
    }
    serializer::prepare_for_serialization();
    write_asset_manager_data_helper<Mesh>(ofs);
    // Write textures before materials since material can reference texture
    write_asset_manager_data_helper<Texture2D>(ofs);
    write_asset_manager_data_helper<Material>(ofs);
    BinarySerializer::write(ofs, this);
    BinarySerializer::write(ofs, &m_entity_manager);
    m_component_manager.write(ofs);

    ofs.close();
    // fix to avoid camera jumps ???
    // again, cursor pos in handler can have huge offset difference at this point
    // InputSystem::instance().cursor_movement_ignore_first_frames();
    Logger::info("Scene has been successfully saved to file {}", file);
  }

  void Scene::load(const std::filesystem::path& file)
  {
    std::ifstream ifs(file, std::ios_base::binary);
    if (!ifs.is_open())
    {
      Logger::error("[Scene::load] Failed to open file {}.", file);
      return;
    }

    std::vector<NodeDeserializationData> nodes_read_data;
    on_node_read_completed +=
        new NamedFunctionListener("NodeReadListener", std::function([&](const NodeDeserializationData& data)
                                                                    { nodes_read_data.push_back(data); }));

    clear();
    serializer::prepare_for_serialization();

    // Keep assets alive until some AssetHandle will reference it later during deserialization
    auto meshes = read_asset_manager_data_helper<Mesh>(ifs);
    // Read textures before materials since material can reference texture
    auto textures = read_asset_manager_data_helper<Texture2D>(ifs);
    auto materials = read_asset_manager_data_helper<Material>(ifs);
    BinarySerializer::read(ifs, this);
    BinarySerializer::read(ifs, &m_entity_manager);
    m_component_manager.read(ifs);
    ifs.close();

    on_node_read_completed.remove_listener_by_name("NodeReadListener");
    SceneGraphBuilder builder;
    builder.build(nodes_read_data);

    // m_camera.update_camera_vectors();
    prepare_scene_for_rendering();
  }

  void Scene::clear()
  {
    m_entity_manager.clear();
    m_component_manager.clear();
    m_entities_with_visible_bboxes.clear();
    m_entities_with_visible_normals.clear();
    TextureManager::clear();
    MeshManager::clear();
    MaterialManager::clear();
  }

  void Scene::clear_geometry()
  {
    for (EntityId entity : m_component_manager.get_entities<ecs::DrawableComponent>())
    {
      m_entity_manager.remove(entity);
      m_component_manager.entity_destroyed(entity);
    }
    m_entities_with_visible_bboxes.clear();
    m_entities_with_visible_normals.clear();
    update_geometry_pipeline();
  }

  void Scene::render_skybox()
  {
    glDepthFunc(GL_LEQUAL);
    Shader* shader = ShaderStorage::get(ShaderStorage::ShaderType::SKYBOX);
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
      if (m_ui.get_item_selection_wheel().is_visible())
      {
        return;
      }
      y = static_cast<int>(m_camera.get_screen_size().y) - y;
      Ray ray = m_camera.cast_ray(x, y);
      std::pair<EntityId, float> hit_info = { 0, INFINITY };
#if DEBUG_RAY
      std::vector<Polyline> rays;
#endif
      for (auto [entity, drawable_component] : m_component_manager.get_components_zipped<ecs::DrawableComponent>())
      {
        const auto& transform = m_component_manager.get_component<ecs::TransformComponent>(entity);
        const glm::mat4 inv_model_mat = glm::inverse(transform.transform_node.get_world_mat());
        Ray ray_local =
            Ray(inv_model_mat * glm::vec4(ray.get_origin(), 1), inv_model_mat * glm::vec4(ray.get_direction(), 0));
        if (auto hit = ray_local.intersect_entity(drawable_component))
        {
          const glm::vec3 ray_hit_pos_world = transform.transform_node.get_world_mat() * glm::vec4(hit->position, 1);
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
            hit_info.first = entity;
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
        tex->resize(width, height);
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

  void Scene::update_transforms_ssbo()
  {
    m_entity_transform_ssbo_idx_map.clear();
    auto components = m_component_manager.get_components_zipped<ecs::TransformComponent>();
    BindGuard bg(&m_transforms_ssbo);
    m_transforms_ssbo.resize_if_smaller(components.size() * sizeof(glm::mat4));
    for (size_t idx = 0; auto [entity, transform] : components)
    {
      m_entity_transform_ssbo_idx_map[entity] = idx;
      m_transforms_ssbo.set_data(&transform.transform_node.get_world_mat(), sizeof(glm::mat4), idx * sizeof(glm::mat4));
      idx++;
    }
  }

  void Scene::update_transform_ssbo_entry(EntityId entity)
  {
    const auto it = m_entity_transform_ssbo_idx_map.find(entity);
    if (it == m_entity_transform_ssbo_idx_map.end())
    {
      Logger::warn("[Scene::update_transform_ssbo]: Entity's {} SSBO slot not found", entity);
      return;
    }
    const uint32_t ssbo_slot_idx = it->second;
    ecs::TransformComponent& transform = m_component_manager.get_component<ecs::TransformComponent>(entity);
    m_transforms_ssbo.bind();
    m_transforms_ssbo.set_data(&transform.transform_node.get_world_mat(), sizeof(glm::mat4),
                               sizeof(glm::mat4) * ssbo_slot_idx);
    m_transforms_ssbo.unbind();
  }

  void Scene::update_lights_ssbo()
  {
    m_entity_light_ssbo_idx_map.clear();
    auto components = m_component_manager.get_components_zipped<ecs::LightComponent>();
    BindGuard bg(&m_lights_ssbo);
    m_lights_ssbo.resize_if_smaller(components.size() * sizeof(ecs::LightComponent));
    for (size_t i = 0; auto [entity, light_component] : components)
    {
      m_entity_light_ssbo_idx_map[entity] = i;
      m_lights_ssbo.set_data(&light_component, sizeof(light_component), i * sizeof(light_component));
      i++;
    }
  }

  void Scene::update_light_ssbo_entry(EntityId entity)
  {
    const auto it = m_entity_light_ssbo_idx_map.find(entity);
    if (it == m_entity_light_ssbo_idx_map.end())
    {
      Logger::warn("[Scene::update_light_ssbo_entry]: Entity's {} SSBO slot not found", entity);
      return;
    }
    const uint32_t ssbo_slot_idx = it->second;
    ecs::LightComponent& light_component = m_component_manager.get_component<ecs::LightComponent>(entity);
    ecs::TransformComponent& transform_component = m_component_manager.get_component<ecs::TransformComponent>(entity);
    light_component.position = transform_component.transform_node.get_world_mat()[3];
    // OpenGL has right handed coordinate system. -Z is a forward vector
    light_component.dir = -transform_component.transform_node.get_world_mat()[2];
    // light_component.shadow_matrix = transform_component.transform_node.get_world_mat();
    m_lights_ssbo.bind();
    m_lights_ssbo.set_data(&light_component, sizeof(light_component), sizeof(light_component) * ssbo_slot_idx);
    m_lights_ssbo.unbind();
  }

  void Scene::handle_keyboard_button_click(InputCode input_code)
  {
    const InputSystem& input_system = InputSystem::instance();
    const InputContext* ic = input_system.get_active_input_context();

    if (input_code == InputCode::FURY_KEY_GRAVE_ACCENT)
    {
      m_ui.get_scene_info_window().toggle_visibility();
      if (!m_ui.get_scene_info_window().is_visible())
      {
        m_ui.get_gizmo().hide();
        m_selected_entity = g_invalid_entity_id;
      }
      return;
    }

    // TODO: input context stores available buttons that can be pressed
    // if (ic->type == InputContextType::SCENE_EDITING_MODE &&
    //    !(input_code == InputCode::FURY_KEY_Q && m_selection_wheel.is_visible()))
    //{
    //  // Following actions can only be triggered in free camera mode
    //  return;
    //}

    switch (input_code)
    {
    // case InputCode::FURY_KEY_ESC:
    //{
    //   m_selected_entity = g_invalid_entity_id;
    //   m_ui.get_component(Gizmo::default_name)->hide();
    // }
    // break;
    case InputCode::FURY_KEY_O:
    {
      m_scene_options.show_shadow_map = !m_scene_options.show_shadow_map;
    }
    break;
    case InputCode::FURY_KEY_P:
    {
      Camera::ProjectionMode current_mode = m_camera.get_projection_mode();
      m_camera.set_projection_mode(current_mode == Camera::ProjectionMode::ORTHOGRAPHIC
                                       ? Camera::ProjectionMode::PERSPECTIVE
                                       : Camera::ProjectionMode::ORTHOGRAPHIC);
    }
    break;
    case InputCode::FURY_KEY_Q:
    {
      ItemSelectionWheel& item_selection_wheel = m_ui.get_item_selection_wheel();
      item_selection_wheel.select(-1);
      item_selection_wheel.toggle_visibility();
    }
    break;
    case InputCode::FURY_KEY_M:
    {
      Frustum fr = m_camera.get_frustum();
      for (const auto& [p1, p2] : fr.debug_lines)
      {
        m_debug_pass.add_line(p1, p2, glm::vec4(0, 1, 0, 1));
      }
    }
    break;
    case InputCode::FURY_KEY_L:
    {
      if (m_selected_entity != g_invalid_entity_id)
      {
        // drop selected object on object below
        EntityId selected_entity = m_selected_entity;
        auto& selected_entity_transform = m_component_manager.get_component<ecs::TransformComponent>(selected_entity);
        auto& selected_entity_dc = m_component_manager.get_component<ecs::DrawableComponent>(selected_entity);
        const auto points = selected_entity_dc.bbox.get_points();

        float distance = INFINITY;
        m_debug_pass.clear_lines();
        for (const glm::vec3& p : points)
        {
          Ray ray(selected_entity_transform.transform_node.get_world_mat() * glm::vec4(p, 1), glm::vec3(0, -1, 0));

          for (auto [entity, drawable_component] : m_component_manager.get_components_zipped<ecs::DrawableComponent>())
          {
            if (entity == selected_entity)
            {
              continue;
            }
            BoundingBox bbox_world;
            const auto& entity_transform = m_component_manager.get_component<ecs::TransformComponent>(entity);
            bbox_world.init(
                entity_transform.transform_node.get_world_mat() * glm::vec4(drawable_component.bbox.min(), 1),
                entity_transform.transform_node.get_world_mat() * glm::vec4(drawable_component.bbox.max(), 1));
            if (auto hit = ray.intersect_aabb(bbox_world))
            {
              m_debug_pass.add_line(ray.get_origin(), hit->position);
              // Logger::info("hit {} distance {}", hit->position, hit->distance);
              distance = std::min(hit->distance, distance);
            }
            if (distance != INFINITY)
            {
              glm::vec3 old = selected_entity_transform.transform_node.get_translation();
              distance = distance - old.y;
              selected_entity_transform.transform_node.set_translation(glm::vec3(old.x, -distance, old.z));
              update_shadow_map();
            }
          }
        }
      }
      break;
    case InputCode::FURY_KEY_BACKSPACE:
    {
      if (m_selected_entity != g_invalid_entity_id)
      {
        remove_entity(m_selected_entity);
        m_selected_entity = g_invalid_entity_id;
        m_ui.get_gizmo().hide();
      }
    }
    break;
    default:
      break;
    }
    }
  }

  void Scene::calculate_scene_bbox()
  {
    m_bbox.reset();
    for (auto [entity, drawable_component] : m_component_manager.get_components_zipped<ecs::DrawableComponent>())
    {
      auto& transform = m_component_manager.get_component<ecs::TransformComponent>(entity);
      transform.transform_node.update();
      if (!drawable_component.bbox)
      {
        drawable_component.bbox = utils::calculate_bbox(drawable_component.mesh_nodes);
      }
      const BoundingBox bbox =
          utils::calculate_world_space_bbox(drawable_component.bbox, transform.transform_node.get_world_mat());
      m_bbox.grow(bbox.min(), bbox.max());
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
    glViewport(0, 0, ::SHADOWMAP_WIDTH, ::SHADOWMAP_HEIGHT);
    glClear(GL_DEPTH_BUFFER_BIT);
    m_shadows_pass.update();
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
    InputSystem::instance().push_context(InputContextType::SCENE_EDITING_MODE);
  }

  void Scene::handle_ui_component_closing()
  {
    InputSystem& is = InputSystem::instance();
    assert(is.get_input_context_stack_size() > 0);
    is.pop_context();
    if (is.get_active_input_context()->type == InputContextType::FREECAM)
    {
      m_camera.unfreeze();
      glfwSetInputMode(m_window->gl_window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
  }

  void Scene::on_entity_normals_visibility_toggle(EntityId entity, bool visible_normals)
  {
    if (visible_normals)
    {
      m_entities_with_visible_normals.insert(entity);
    }
    else
    {
      m_entities_with_visible_normals.erase(entity);
    }
    m_debug_pass.update_normals();
  }

  void Scene::on_entity_bbox_visibility_toggle(EntityId entity, bool visible_bbox)
  {
    if (visible_bbox)
    {
      m_entities_with_visible_bboxes.insert(entity);
    }
    else
    {
      m_entities_with_visible_bboxes.erase(entity);
    }
  }

  void Scene::remove_entity(EntityId entity)
  {
    m_entity_manager.remove(entity);
    m_component_manager.entity_destroyed(entity);
    if (auto it = m_entities_with_visible_bboxes.find(entity); it != m_entities_with_visible_bboxes.end())
    {
      m_entities_with_visible_bboxes.erase(it);
    }
    if (auto it = m_entities_with_visible_normals.find(entity); it != m_entities_with_visible_normals.end())
    {
      m_entities_with_visible_normals.erase(it);
    }
    m_update_geometry_pipeline = true;
  }

  std::tuple<EntityId, ecs::DrawableComponent&, ecs::TransformComponent&>
  Scene::add_entity(const std::vector<MeshNode>& mesh_nodes, bool has_surface)
  {
    EntityId entity = m_entity_manager.create();
    ecs::DrawableComponent& drawable_component = m_component_manager.add_component<ecs::DrawableComponent>(entity);
    ecs::TransformComponent& transform_component = m_component_manager.add_component<ecs::TransformComponent>(entity);
    drawable_component.mesh_nodes = mesh_nodes;
    transform_component.transform_node.set_entity(entity);
    if (has_surface)
    {
      utils::set_flag(drawable_component.flags, DrawableFlags::HAS_SURFACE);
    }
    m_update_geometry_pipeline = true;
    return std::make_tuple(entity, std::ref(drawable_component), std::ref(transform_component));
  }

  void Scene::update_geometry_pipeline()
  {
    update_lights_ssbo();
    update_transforms_ssbo();
    for (RenderPass* rp : m_render_passes)
    {
      rp->update();
    }
    update_shadow_map();
    calculate_scene_bbox();
  }

  void Scene::create_default_lights()
  {
    const EntityId dir_light = m_entity_manager.create();
    ecs::LightComponent& dir_light_component = m_component_manager.add_component<ecs::LightComponent>(dir_light);
    ecs::TransformComponent& dir_light_transform =
        m_component_manager.add_component<ecs::TransformComponent>(dir_light);
    dir_light_component.is_active = true;
    dir_light_component.type = ecs::LightType::DIRECTIONAL;
    dir_light_transform.transform_node.set_entity(dir_light);

    // center is in world space
    const glm::vec3 bbox_center = m_bbox.center();
    const glm::vec3 dir_light_position = glm::vec3(bbox_center.x + 0.12, bbox_center.y + 0.33, bbox_center.z + 0.7);
    const glm::vec3 dir_light_target = bbox_center;
    constexpr float cube_bound = 6.f;
    dir_light_component.dir = glm::vec4(glm::normalize(dir_light_target - dir_light_position), 1);
    dir_light_component.position = glm::vec4(dir_light_position + cube_bound * -glm::vec3(dir_light_component.dir), 1);
    const glm::mat4 proj_mat = glm::ortho<float>(-cube_bound, cube_bound, -cube_bound, cube_bound, -10.f, 10.f);
    const glm::mat4 view_mat = glm::lookAt(dir_light_position, dir_light_target, glm::vec3(0, 1, 0));
    dir_light_component.shadow_matrix = proj_mat * view_mat;
    dir_light_transform.transform_node.set_translation(dir_light_component.position);

    // spot light from camera position
    const EntityId spot_light = m_entity_manager.create();
    ecs::LightComponent& spot_light_component = m_component_manager.add_component<ecs::LightComponent>(spot_light);
    ecs::TransformComponent& spot_light_transform =
        m_component_manager.add_component<ecs::TransformComponent>(spot_light);
    spot_light_component.type = ecs::LightType::SPOT;
    spot_light_component.position = glm::vec4(m_camera.get_position(), 1);
    spot_light_transform.transform_node.set_parent(&m_camera.get_transform_component().transform_node);
    spot_light_transform.transform_node.set_entity(spot_light);
    spot_light_component.is_active = true;
    // Force adding it to pool of dirty nodes and sync its position with camera's on first frame
    spot_light_transform.transform_node.mark_dirty();
  }

  void Scene::create_default_scene()
  {
    clear();

    //m_camera.set_
    //m_camera.init_as_entity(this);
    m_camera.set_position(glm::vec3(-4.f, 2.f, 3.f));
    m_camera.look_at(glm::vec3(2.f, 0.5f, 0.5f));

    {
      Vertex arr[6];
      arr[0].position = glm::vec3(0.f, 1.f, 0.f), arr[0].color = glm::vec4(0.f, 1.f, 0.f, 1.f);
      arr[1].position = glm::vec3(0.f, 0.f, 0.f), arr[1].color = glm::vec4(0.f, 1.f, 0.f, 1.f);
      arr[2].position = glm::vec3(0.f, 0.f, 0.f), arr[2].color = glm::vec4(1.f, 0.f, 0.f, 1.f);
      arr[3].position = glm::vec3(1.f, 0.f, 0.f), arr[3].color = glm::vec4(1.f, 0.f, 0.f, 1.f);
      arr[4].position = glm::vec3(0.f, 0.f, 0.f), arr[4].color = glm::vec4(0.f, 0.f, 1.f, 1.f);
      arr[5].position = glm::vec3(0.f, 0.f, 1.f), arr[5].color = glm::vec4(0.f, 0.f, 1.f, 1.f);

      AssetHandle<Mesh> mesh = MeshManager::create();
      for (int i = 0; i < 6; i++)
      {
        mesh.asset->append_vertex(arr[i]);
      }
      auto [entity, drawable_component, transform_component] = add_entity({ MeshNode { mesh } }, false);
      transform_component.transform_node.set_entity(entity);
      drawable_component.render_config =
          RenderConfig { .mode = GL_LINE_STRIP, .use_indices = false, .use_color_from_vertex_attribute = true };
      utils::clear_flag(drawable_component.flags, DrawableFlags::HAS_SURFACE);
    }

    {
      ecs::IcosahedronParametersComponent params;
      params.subdivisiton_depth = 4;
      params.project_points_on_sphere = true;
      const std::string lookup_name = utils::generate_asset_lookup_name(
          "Icosahedron", ShadingMode::NO_SHADING, params.subdivisiton_depth, params.project_points_on_sphere);
      AssetHandle<Mesh> mesh = MeshManager::get_or_create(
          lookup_name, [&]() { return GeometryFactory::create_icosahedron(params, ShadingMode::NO_SHADING); });
      auto [entity, drawable_component, transform_component] = add_entity({ MeshNode { mesh } }, true);
      drawable_component.color = glm::vec4(1, 1, 0, 1);
      drawable_component.shading_mode = ShadingMode::NO_SHADING;
      transform_component.transform_node.set_translation(glm::vec3(0.f, 0.6f, 2.f));
      transform_component.transform_node.set_scale(glm::vec3(0.3f));
      m_component_manager.add_component(entity, params);
    }

    {
      ecs::IcosahedronParametersComponent params;
      params.subdivisiton_depth = 4;
      params.project_points_on_sphere = true;
      const std::string lookup_name = utils::generate_asset_lookup_name(
          "Icosahedron", ShadingMode::SMOOTH_SHADING, params.subdivisiton_depth, params.project_points_on_sphere);
      AssetHandle<Mesh> mesh = MeshManager::get_or_create(
          lookup_name, [&]() { return GeometryFactory::create_icosahedron(params, ShadingMode::SMOOTH_SHADING); });
      auto [entity, drawable_component, transform_component] = add_entity({ MeshNode { mesh } }, true);
      drawable_component.color = glm::vec4(1.f, 0.f, 0.f, 1.f);
      drawable_component.shading_mode = ShadingMode::SMOOTH_SHADING;
      transform_component.transform_node.set_translation(glm::vec3(2.5f, 0.5f, 2.f));
      transform_component.transform_node.set_scale(glm::vec3(0.3f));
      m_component_manager.add_component(entity, params);
    }

    {
      ecs::CubeParametersComponent params;
      const std::string lookup_name = utils::generate_asset_lookup_name("Cube", ShadingMode::FLAT_SHADING);
      AssetHandle<Mesh> mesh = MeshManager::get_or_create(
          "Cube", [&]() { return GeometryFactory::create_cube(params, ShadingMode::FLAT_SHADING); });
      auto [entity, drawable_component, transform_component] = add_entity({ MeshNode { mesh } }, true);
      drawable_component.shading_mode = ShadingMode::FLAT_SHADING;
      auto brick_material = MaterialManager::create();
      brick_material.asset->diffuse_tex = TextureManager::load(utils::get_textures_folder() / "brick.jpg");
      drawable_component.mesh_nodes[0].material = brick_material;
      transform_component.transform_node.set_translation(glm::vec3(0.25f));
      transform_component.transform_node.set_scale(glm::vec3(0.5f));
      m_component_manager.add_component(entity, params);
    }

    {
      ecs::CubeParametersComponent params;
      const std::string lookup_name = utils::generate_asset_lookup_name("Cube", ShadingMode::FLAT_SHADING);
      AssetHandle<Mesh> mesh = MeshManager::get_or_create(
          lookup_name, [&]() { return GeometryFactory::create_cube(params, ShadingMode::FLAT_SHADING); });
      auto [entity, drawable_component, transform_component] = add_entity({ MeshNode { mesh } }, true);
      drawable_component.shading_mode = ShadingMode::SMOOTH_SHADING;
      drawable_component.color = glm::vec4(0.4f, 1.f, 0.4f, 1.f);
      transform_component.transform_node.set_translation(glm::vec3(1.25f, 1.f, 1.f));
      m_component_manager.add_component(entity, params);
      m_entities_with_visible_normals.insert(entity);
    }

    {
      ecs::PyramidParametersComponent params;
      const std::string lookup_name = utils::generate_asset_lookup_name("Pyramid", ShadingMode::FLAT_SHADING);
      AssetHandle<Mesh> mesh = MeshManager::get_or_create(
          lookup_name, [&]() { return GeometryFactory::create_pyramid(params, ShadingMode::FLAT_SHADING); });
      auto [entity, drawable_component, transform_component] = add_entity({ MeshNode { mesh } }, true);
      drawable_component.shading_mode = ShadingMode::FLAT_SHADING;
      drawable_component.color = glm::vec4(0.976f, 0.212f, 0.98f, 1.f);
      transform_component.transform_node.set_translation(glm::vec3(0.75f, 0.65f, 2.25f));
      transform_component.transform_node.set_scale(glm::vec3(0.5f));
      m_component_manager.add_component(entity, params);
      // Make it rotating
      m_component_manager.add_component(entity, ecs::RotationComponent({ 1, 0, 0 }, glm::radians(90.f)));
    }

    {
      ecs::BezierCurveParametersComponent params;
      params.curve_type = BezierCurveType::Quadratic;
      params.start_point = { 0, 0, 0 };
      params.end_point = glm::vec3(2.5f, 0.f, 0.f);
      params.control_point1 = glm::vec3(1.25f, 2.f, 0.f);
      AssetHandle<Mesh> mesh =
          MeshManager::get_or_create("BezierCurve1", [&]() { return GeometryFactory::create_bezier_curve(params); });
      auto [entity, drawable_component, transform_component] = add_entity({ MeshNode { mesh } }, false);
      drawable_component.color = glm::vec4(1.f, 0.f, 0.f, 1.f);
      drawable_component.render_config.mode = GL_LINE_STRIP;
      drawable_component.render_config.use_indices = false;
      m_component_manager.add_component(entity, params);
    }

    {
      ecs::BezierCurveParametersComponent params;
      params.curve_type = BezierCurveType::Cubic;
      params.start_point = { 0, 0, 0 };
      params.end_point = glm::vec3(0.f, 0.f, -2.5f);
      params.control_point1 = glm::vec3(0.f, 2.f, -1.25f);
      params.control_point2 = glm::vec3(0.f, -2.f, -1.75f);
      AssetHandle<Mesh> mesh =
          MeshManager::get_or_create("BezierCurve2", [&]() { return GeometryFactory::create_bezier_curve(params); });
      auto [entity, drawable_component, transform_component] = add_entity({ MeshNode { mesh } }, false);
      drawable_component.render_config.mode = GL_LINE_STRIP;
      drawable_component.render_config.use_indices = false;
      m_component_manager.add_component(entity, params);
    }

    prepare_scene_for_rendering();
  }

  void Scene::prepare_scene_for_rendering()
  {
    // Must calculate scene bbox before create_default_lights() call since it calculates directional light
    // position based on scene's bbox.
    calculate_scene_bbox();
    if (m_component_manager.get_entities<ecs::LightComponent>().empty())
    {
      create_default_lights();
    }
    update_transforms_ssbo();
    update_lights_ssbo();
    for (RenderPass* render_pass : m_render_passes)
    {
      render_pass->update();
    }
    update_shadow_map();

    for (auto [entity, transform] : m_component_manager.get_components_zipped<ecs::TransformComponent>())
    {
      if (transform.transform_node.get_entity() == g_invalid_entity_id)
      {
        Logger::warn("Transform node had no entity assigned! Assigning entity {} to it...", entity);
        transform.transform_node.set_entity(entity);
      }
    }
    // m_camera.update_camera_vectors();

    // fix to avoid camera jumps ???
    // again, cursor pos in handler can have huge offset difference at this point
    // InputSystem::instance().cursor_movement_ignore_first_frames();
  }

  void Scene::select_object(EntityId entity, bool click_from_menu_item)
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
      const auto& transform = m_component_manager.get_component<ecs::TransformComponent>(entity);
      const auto& dc = m_component_manager.get_component<ecs::DrawableComponent>(entity);
      // look at selected object
      m_camera.look_at(dc.bbox.center() + glm::vec3(transform.transform_node.get_world_mat()[3]));
    }
    // for now support only single object selection
    m_selected_entity = entity;
    m_ui.get_gizmo().show();
  }

  void Scene::tick(float dt)
  {
    m_cam_controller.tick(dt);
    for (auto [entity, rot_comp] : m_component_manager.get_components_zipped<ecs::RotationComponent>())
    {
      auto& transform_component = m_component_manager.get_component<ecs::TransformComponent>(entity);
      const glm::quat& q = transform_component.transform_node.get_rotation();
      transform_component.transform_node.set_rotation(glm::rotate(q, rot_comp.angle * dt, rot_comp.axis));
    }

    // TODO: Optimize. Store array of dirty components instead of iterating over all of them.
    for (auto [entity, transform_component] : m_component_manager.get_components_zipped<ecs::TransformComponent>())
    {
      if (transform_component.transform_node.is_dirty())
      {
        auto& dirty_tag = m_component_manager.get_or_create_component<ecs::DirtyEntityTagComponent>(entity);
        dirty_tag.dirty_bits.transform_changed = true;
      }
    }

    bool updated_scene = false;
    bool updated_shading_mode_change = false;
    for (auto [entity, dirty_tag] : m_component_manager.get_components_zipped<ecs::DirtyEntityTagComponent>())
    {
      if (dirty_tag.dirty_bits.color_changed)
      {
        m_geometry_pass.update_mesh_data(entity);
      }
      if (dirty_tag.dirty_bits.shading_mode_changed && !updated_shading_mode_change)
      {
        m_debug_pass.update_normals();
        m_geometry_pass.update();
        updated_shading_mode_change = true;
      }

      if (dirty_tag.dirty_bits.transform_changed)
      {
        auto& transform_component = m_component_manager.get_component<ecs::TransformComponent>(entity);
        transform_component.transform_node.update();
        update_transform_ssbo_entry(entity);
        if (!updated_scene)
        {
          calculate_scene_bbox();
          update_shadow_map();
          updated_scene = true;
        }
      }
      if (m_component_manager.get_component_safe<ecs::LightComponent>(entity) != nullptr)
      {
        update_light_ssbo_entry(entity);
      }
    }
    m_component_manager.clear_components<ecs::DirtyEntityTagComponent>();

    if (m_update_geometry_pipeline)
    {
      update_geometry_pipeline();
      m_update_geometry_pipeline = false;
    }

    auto vm = m_camera.get_view_matrix();
    m_camera_data_ubo.bind();
    m_camera_data_ubo.set_data(&vm, sizeof(glm::mat4), 0);
    m_camera_data_ubo.set_data(&m_camera.get_projection_matrix(), sizeof(glm::mat4), sizeof(glm::mat4));
    m_camera_data_ubo.unbind();
  }

  void Scene::enqueue_job_from_async_thread(std::move_only_function<void()> job)
  {
    std::lock_guard lg(m_async_tasks_mutex);
    m_async_tasks.push_back(std::move(job));
  }

} // namespace fury
