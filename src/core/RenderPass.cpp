#include "RenderPass.hpp"
#include "WindowGLFW.hpp"
#include "BindGuard.hpp"
#include "Shader.hpp"
#include "ShaderStorage.hpp"
#include "Scene.hpp"
#include "Vertex.hpp"
#include "core/Material.hpp"
#include "core/Frustum.hpp"
#include "ecs/components/TransformComponent.hpp"
#include "ecs/components/LightComponent.hpp"
#include "ecs/components/DrawableComponent.hpp"
#include "ecs/components/RotationComponent.hpp"
#include "ui/ItemSelectionWheel.hpp"
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <numeric>

namespace
{
  void set_default_vertex_attributes(fury::VertexArrayObject& vao)
  {
    vao.link_attrib(0, 3, GL_FLOAT, sizeof(fury::Vertex), nullptr);                       // position
    vao.link_attrib(1, 3, GL_FLOAT, sizeof(fury::Vertex), (void*)(sizeof(GLfloat) * 3));  // normal
    vao.link_attrib(2, 4, GL_FLOAT, sizeof(fury::Vertex), (void*)(sizeof(GLfloat) * 6));  // color
    vao.link_attrib(3, 2, GL_FLOAT, sizeof(fury::Vertex), (void*)(sizeof(GLfloat) * 10)); // texture
  }
  // constexpr static GLuint cube_lines_indices[24] = { 0, 1, 1, 2, 2, 3, 3, 0, // front
  //                                                    4, 5, 5, 6, 6, 7, 7, 4, // back
  //                                                    0, 4, 3, 7, 1, 5, 2, 6 };
  //
  // constexpr static float unit_cube[24] = { -0.5, -0.5, 0.5,  0.5, -0.5, 0.5,  0.5, 0.5, 0.5,  -0.5, 0.5, 0.5,
  //                                          -0.5, -0.5, -0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, -0.5 };
} // namespace

namespace fury
{
  RenderPass::RenderPass(Scene* scene)
  {
    init(scene);
  }

  void RenderPass::init(Scene* scene)
  {
    m_scene = scene;
  }

  void GeometryPass::update_mesh_data(EntityId entity)
  {
    auto it = m_render_offsets.find(entity);
    if (it == m_render_offsets.end())
    {
      return;
    }
    const std::vector<MeshRenderOffsets>& offsets = it->second;
    const auto& dc = m_scene->get_component_manager().get_component<ecs::DrawableComponent>(entity);
    BindGuard bg(dc.render_config.use_indices ? &m_vbo_indices : &m_vbo_arrays);
    assert(offsets.size() == dc.mesh_nodes.size());
    for (int i = 0; const MeshRenderOffsets& mesh_offset : offsets)
    {
      const Ref<Mesh>& mesh = dc.mesh_nodes[i++].mesh.asset;
      if (dc.render_config.use_indices)
      {
        m_vbo_indices.set_data(mesh->get_vertices().data(), mesh->get_vertices().size() * sizeof(Vertex),
                               mesh_offset.vbo_indices_offset);
      }
      else
      {
        m_vbo_arrays.set_data(mesh->get_vertices().data(), mesh->get_vertices().size() * sizeof(Vertex),
                              mesh_offset.vbo_arrays_offset);
      }
    }
  }

  void GeometryPass::allocate_memory_for_buffers()
  {
    size_t vcount_vbo_indices = 0;
    size_t vcount_vbo_arrays = 0;
    size_t idx_count = 0;

    const auto& components = m_scene->get_component_manager().get_components<ecs::DrawableComponent>();
    for (const auto& component : components)
    {
      for (const auto& mesh_node : component.mesh_nodes)
      {
        if (component.render_config.use_indices)
        {
          assert(component.render_config.mode == GL_TRIANGLES);
          vcount_vbo_indices += mesh_node.mesh.asset->get_vertices().size();
          idx_count += mesh_node.mesh.asset->get_faces().size() * 3;
        }
        else
        {
          vcount_vbo_arrays += mesh_node.mesh.asset->get_vertices().size();
        }
      }
    }
    m_vbo_arrays.bind();
    m_vbo_arrays.resize_if_smaller(vcount_vbo_arrays * sizeof(Vertex));
    m_vbo_arrays.unbind();
    m_vbo_indices.bind();
    m_vbo_indices.resize_if_smaller(vcount_vbo_indices * sizeof(Vertex));
    m_vbo_indices.unbind();
    m_ebo.bind();
    m_ebo.resize_if_smaller(idx_count * sizeof(GLuint));
    m_ebo.unbind();
  }

  void GeometryPass::split_objects()
  {
    // split objects on those that use indices for rendering and those that use just vertices,
    // so that during rendering we can bind needed vao once instead of switching between them on each loop iteration
    m_objects_indices_rendering_mode.clear();
    m_objects_arrays_rendering_mode.clear();
    const auto& drawable_entities = m_scene->get_component_manager().get_entities<ecs::DrawableComponent>();
    const auto& drawable_components = m_scene->get_component_manager().get_components<ecs::DrawableComponent>();
    assert(drawable_entities.size() == drawable_components.size());
    m_objects_indices_rendering_mode.reserve(drawable_entities.size());
    m_objects_arrays_rendering_mode.reserve(drawable_entities.size());
    for (size_t i = 0; i < drawable_entities.size(); i++)
    {
      if (drawable_components[i].render_config.use_indices)
      {
        m_objects_indices_rendering_mode.push_back(drawable_entities[i]);
      }
      else
      {
        m_objects_arrays_rendering_mode.push_back(drawable_entities[i]);
      }
    }
  }

  void GeometryPass::render_entities(const std::vector<EntityId>& entities, const Frustum& frustum)
  {
    const Camera& camera = m_scene->get_camera();
    for (const EntityId entity : entities)
    {
      const auto& transform_component = m_scene->get_component_manager().get_component<ecs::TransformComponent>(entity);
      const auto& dc = m_scene->get_component_manager().get_component<ecs::DrawableComponent>(entity);
      if (m_scene->get_options().is_frustum_culling_enabled)
      {
        const glm::mat4& transform = transform_component.transform_node.get_world_mat();
        BoundingBox bbox = dc.bbox;
        const glm::vec3& min = bbox.min();
        const glm::vec3& max = bbox.max();
        const glm::vec3 center = bbox.center();
        const glm::vec3 extents = (max - min) * 0.5f;
        const glm::mat3 mm = { glm::abs(transform[0]), glm::abs(transform[1]), glm::abs(transform[2]) };
        const glm::vec3 center_world = transform * glm::vec4(center, 1);
        const glm::vec3 extents_world = mm * extents;
        // approximated AABB if transform has rotation
        bbox.init(center_world - extents_world, center_world + extents_world);
        if (!frustum.is_inside(bbox))
        {
          m_num_culled_objects++;
          continue;
        }
      }

      std::vector<std::string> defines;
      if (dc.render_config.use_color_from_vertex_attribute)
      {
        defines.emplace_back("USE_VERTEX_COLOR");
      }
      if (dc.shading_mode != ShadingMode::NO_SHADING)
      {
        defines.emplace_back("APPLY_LIGHTING");
      }

      if (m_scene->get_selected_entity() == entity && utils::get_flag(dc.flags, DrawableFlags::HAS_SURFACE))
      {
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilMask(0xFF);
      }

      static Material fallback_material;
      const std::vector<MeshRenderOffsets>& meshes_offsets = m_render_offsets.at(entity);
      for (size_t idx = 0; const auto& mesh_node : dc.mesh_nodes)
      {
        int mesh_defines_added = 0;
        utils::ScopeGuard guard;
        guard.set_function(
            [&mesh_defines_added, &defines]()
            {
              for (int i = 0; i < mesh_defines_added; i++)
              {
                defines.pop_back();
              }
            });
        const Material* material = mesh_node.material ? mesh_node.material.asset.get() : &fallback_material;
        const Ref<Mesh>& mesh = mesh_node.mesh.asset;

        if (material->ambient_tex)
        {
          defines.emplace_back("HAS_AMBIENT_TEXTURE");
          glActiveTexture(GL_TEXTURE0);
          glBindTexture(GL_TEXTURE_2D, material->ambient_tex.asset->id());
          mesh_defines_added++;
        }

        if (material->diffuse_tex)
        {
          defines.emplace_back("HAS_DIFFUSE_TEXTURE");
          glActiveTexture(GL_TEXTURE0 + 1);
          glBindTexture(GL_TEXTURE_2D, material->diffuse_tex.asset->id());
          mesh_defines_added++;
        }

        if (material->specular_tex)
        {
          defines.emplace_back("HAS_SPECULAR_TEXTURE");
          glActiveTexture(GL_TEXTURE0 + 2);
          glBindTexture(GL_TEXTURE_2D, material->specular_tex.asset->id());
          mesh_defines_added++;
        }

        Shader* shader = ShaderStorage::get(ShaderStorage::ShaderType::DEFAULT, defines);
        shader->bind();
        shader->set_vec3("viewPos", camera.get_position());
        shader->set_int(
            "numLights",
            static_cast<int>(m_scene->get_component_manager().get_components<ecs::LightComponent>().size()));
        glActiveTexture(GL_TEXTURE0 + 3);
        glBindTexture(GL_TEXTURE_2D, m_shadow_map_texture);
        shader->set_matrix4f("modelMatrix", transform_component.transform_node.get_world_mat());
        shader->set_vec4("overrideColor", dc.color);

        // set mesh material
        shader->set_vec3("material.ambient", material->ambient);
        shader->set_vec3("material.diffuse", material->diffuse);
        shader->set_vec3("material.specular", material->specular);
        shader->set_float("material.shininess", material->shininess);
        shader->set_float("material.alpha", material->alpha);

        const MeshRenderOffsets& mesh_offsets = meshes_offsets[idx++];
        if (dc.render_config.use_indices)
        {
          glDrawElementsBaseVertex(dc.render_config.mode, mesh->faces_as_indices().size(), GL_UNSIGNED_INT,
                                   (void*)mesh_offsets.ebo_offset, mesh_offsets.basev);
        }
        else
        {
          glDrawArrays(dc.render_config.mode, static_cast<GLint>(mesh_offsets.vbo_arrays_offset),
                       static_cast<GLsizei>(mesh->get_vertices().size()));
        }
        // disable stencil buffer writing
        glStencilFunc(GL_ALWAYS, 0, 0xFF);
        glStencilMask(0x00);
        shader->unbind();
      }
    }
  }

  void GeometryPass::render_selected_objects()
  {
    const EntityId selected_entity = m_scene->get_selected_entity();
    if (selected_entity == g_invalid_entity_id)
    {
      return;
    }

    Shader* shader = ShaderStorage::get(ShaderStorage::ShaderType::OUTLINING);
    shader->bind();

    // glDisable(GL_DEPTH_TEST);
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    glStencilMask(0x00);

    const auto& dc = m_scene->get_component_manager().get_component<ecs::DrawableComponent>(selected_entity);
    const auto& transform_component =
        m_scene->get_component_manager().get_component<ecs::TransformComponent>(selected_entity);
    glm::vec3 scale = transform_component.transform_node.get_scale();
    utils::ScopeGuard sg;
    if (utils::get_flag(dc.flags, DrawableFlags::HAS_SURFACE))
    {
      scale += 0.05;
    }
    else
    {
      if (dc.render_config.mode == GL_LINE || dc.render_config.mode == GL_LINES ||
          dc.render_config.mode == GL_LINE_STRIP || dc.render_config.mode == GL_LINE_LOOP)
      {
        glLineWidth(5);
        sg.set_function([]() { glLineWidth(1); });
      }
    }
    glm::mat4 selected_world_mat = glm::mat4(1.f);
    selected_world_mat = glm::translate(selected_world_mat, transform_component.transform_node.get_translation());
    selected_world_mat = selected_world_mat * glm::toMat4(transform_component.transform_node.get_rotation());
    selected_world_mat = glm::scale(selected_world_mat, scale);

    shader->set_matrix4f("modelMatrix", selected_world_mat);
    const std::vector<MeshRenderOffsets>& meshes_offsets = m_render_offsets.at(selected_entity);
    for (size_t i = 0; const auto& mesh_node : dc.mesh_nodes)
    {
      const Ref<Mesh>& mesh = mesh_node.mesh.asset;
      const MeshRenderOffsets& mesh_offsets = meshes_offsets[i++];
      if (dc.render_config.use_indices)
      {
        BindGuard bg(m_vao_indices);
        glDrawElementsBaseVertex(dc.render_config.mode, mesh->faces_as_indices().size(), GL_UNSIGNED_INT,
                                 (void*)mesh_offsets.ebo_offset, mesh_offsets.basev);
      }
      else
      {
        BindGuard bg(m_vao_arrays);
        glDrawArrays(dc.render_config.mode, static_cast<GLint>(mesh_offsets.vbo_arrays_offset),
                     static_cast<GLsizei>(mesh->get_vertices().size()));
      }
    }
    glStencilMask(0xFF);
    glStencilFunc(GL_ALWAYS, 0, 0xFF);
    glEnable(GL_DEPTH_TEST);
    shader->unbind();
  }

  GeometryPass::GeometryPass()
  {
    BindChainFIFO bind_chain({ &m_vao_indices, &m_vbo_indices });
    ::set_default_vertex_attributes(m_vao_indices);
    BindChainFIFO bind_chain2({ &m_vao_arrays, &m_vbo_arrays });
    ::set_default_vertex_attributes(m_vao_arrays);
  }

  void GeometryPass::update()
  {
    if (m_scene->get_entity_manager().get_entities().empty())
    {
      // clear struct in case if all objects from scene have been deleted,
      // because shadow map pass uses same structs for it's pass
      m_render_offsets.clear();
      m_objects_arrays_rendering_mode.clear();
      m_objects_indices_rendering_mode.clear();
      return;
    }

    allocate_memory_for_buffers();
    split_objects();
    m_render_offsets.clear();

    size_t vbo_indices_offset = 0;
    size_t vbo_arrays_offset = 0;
    size_t ebo_offset = 0;
    size_t basev = 0;

    for (auto [entity, drawable_component] :
         m_scene->get_component_manager().get_components_zipped<ecs::DrawableComponent>())
    {
      size_t model_vertices_within_indices = 0;
      std::vector<MeshRenderOffsets>& meshes_offsets = m_render_offsets[entity];
      for (const auto& mesh_node : drawable_component.mesh_nodes)
      {
        const Ref<Mesh>& mesh = mesh_node.mesh.asset;
        MeshRenderOffsets& mesh_offsets = meshes_offsets.emplace_back();
        const size_t vsize = mesh->get_vertices().size() * sizeof(Vertex);
        const size_t idx_size = mesh->faces_as_indices().size() * sizeof(GLuint);
        if (drawable_component.render_config.use_indices)
        {
          BindChainFIFO bc({ &m_vao_indices, &m_vbo_indices, &m_ebo });
          m_vbo_indices.set_data(mesh->get_vertices().data(), vsize, vbo_indices_offset);
          m_ebo.set_data(mesh->faces_as_indices().data(), idx_size, ebo_offset);
          mesh_offsets.vbo_indices_offset = vbo_indices_offset;
          mesh_offsets.ebo_offset = ebo_offset;
          mesh_offsets.basev = basev;
          basev += mesh->get_vertices().size();
          vbo_indices_offset += vsize;
          ebo_offset += idx_size;
        }
        else
        {
          BindChainFIFO bc({ &m_vao_arrays, &m_vbo_arrays });
          m_vbo_arrays.set_data(mesh->get_vertices().data(), vsize, vbo_arrays_offset);
          // index offset (not in bytes)
          mesh_offsets.vbo_arrays_offset = vbo_arrays_offset / sizeof(Vertex);
          vbo_arrays_offset += vsize;
        }
      }
    }
  }

  void GeometryPass::tick(float)
  {
    m_num_culled_objects = 0;
    Frustum frustum;
    m_scene->get_lights_ssbo().bind_to_index();
    if (m_scene->get_options().is_frustum_culling_enabled)
    {
      frustum = m_scene->get_camera().get_frustum();
    }
    {
      BindGuard bg(m_vao_indices);
      render_entities(m_objects_indices_rendering_mode, frustum);
    }
    {
      BindGuard bg(m_vao_arrays);
      render_entities(m_objects_arrays_rendering_mode, frustum);
    }
    render_selected_objects();
  }

  void ShadowsPass::update()
  {
    const auto& lights = m_scene->get_component_manager().get_components<ecs::LightComponent>();
    auto it = std::find_if(lights.begin(), lights.end(),
                           [](const ecs::LightComponent& c) { return c.type == ecs::LightType::DIRECTIONAL; });
    if (it == lights.end())
    {
      return;
    }
    const ecs::LightComponent& dir_light_component = *it;
    Shader* shader = ShaderStorage::get(ShaderStorage::ShaderType::SHADOW_MAP);
    shader->bind();
    shader->set_matrix4f("lightViewProjMatrix", dir_light_component.shadow_matrix);
    // TODO: change as in Geometry pass
    for (int i = 0; i < 2; i++)
    {
      const EntityId* pentity = nullptr;
      size_t size = 0;
      if (i == 0)
      {
        if (m_gp->m_objects_indices_rendering_mode.empty())
        {
          continue;
        }
        m_gp->m_vao_indices.bind();
        size = m_gp->m_objects_indices_rendering_mode.size();
        pentity = m_gp->m_objects_indices_rendering_mode.data();
      }
      else
      {
        if (m_gp->m_objects_arrays_rendering_mode.empty())
        {
          continue;
        }
        m_gp->m_vao_arrays.bind();
        size = m_gp->m_objects_arrays_rendering_mode.size();
        pentity = m_gp->m_objects_arrays_rendering_mode.data();
      }

      for (size_t obj_i = 0; obj_i < size; obj_i++)
      {
        const EntityId entity = pentity[obj_i];
        shader->set_matrix4f("modelMatrix", m_scene->get_component_manager()
                                                .get_component<ecs::TransformComponent>(entity)
                                                .transform_node.get_world_mat());
        const auto& dc = m_scene->get_component_manager().get_component<ecs::DrawableComponent>(entity);
        const std::vector<GeometryPass::MeshRenderOffsets>& meshes_offsets = m_gp->m_render_offsets.at(entity);
        for (size_t i = 0; const auto& mesh_node : dc.mesh_nodes)
        {
          const GeometryPass::MeshRenderOffsets& mesh_offsets = meshes_offsets[i++];
          const Mesh& mesh = *mesh_node.mesh.asset.get();
          if (dc.render_config.use_indices)
          {
            glDrawElementsBaseVertex(dc.render_config.mode, mesh.faces_as_indices().size(), GL_UNSIGNED_INT,
                                     (void*)mesh_offsets.ebo_offset, mesh_offsets.basev);
          }
          else
          {
            glDrawArrays(dc.render_config.mode, static_cast<GLint>(mesh_offsets.vbo_arrays_offset),
                         static_cast<GLsizei>(mesh.get_vertices().size()));
          }
        }
      }
    }
    m_gp->m_vao_indices.unbind();
    m_gp->m_vao_arrays.unbind();
    shader->unbind();
  }

  void ShadowsPass::tick(float)
  {
  }

  void DebugPass::init(Scene* scene)
  {
    RenderPass::init(scene);

    // Lines
    {
      BindChainFIFO bc({ &m_lines_vao, &m_lines_vbo });
      m_lines_vao.link_attrib(0, 3, GL_FLOAT, sizeof(DebugVertex), 0);
      m_lines_vao.link_attrib(1, 4, GL_FLOAT, sizeof(DebugVertex),
                              reinterpret_cast<void*>(offsetof(DebugVertex, color)));
    }

    // AABBs
    {
      m_bbox_min_max_points_ssbo.set_binding_point(1);
    }

    // Normals
    {
      BindChainFIFO bc({ &m_normals_vao, &m_normals_vbo });
      ::set_default_vertex_attributes(m_normals_vao);
      m_normals_transform_indices_ssbo.set_binding_point(2);
    }
  }

  void DebugPass::update()
  {
    update_normals();
  }

  void DebugPass::tick(float)
  {
    // Lines
    if (!m_lines_vertices.empty())
    {
      assert(m_lines_vertices.size() % 2 == 0);
      Shader* shader = ShaderStorage::get(ShaderStorage::ShaderType::SIMPLE_WITH_VCOLOR);
      BindChainFIFO bc({ shader, &m_lines_vao });
      shader->set_matrix4f("modelMatrix", glm::mat4(1.f));
      glDrawArrays(GL_LINES, 0, m_lines_vertices.size());
    }

    // Bboxes
    if (!m_scene->get_entities_with_visible_bboxes().empty() || m_scene->get_options().show_scene_bbox)
    {
      const size_t n =
          m_scene->get_entities_with_visible_bboxes().size() + static_cast<int>(m_scene->get_options().show_scene_bbox);
      std::vector<AABBBoundsGPU> aabb_bounds;
      aabb_bounds.reserve(n);
      for (EntityId entity : m_scene->get_entities_with_visible_bboxes())
      {
        auto& drawable_component = m_scene->get_component_manager().get_component<ecs::DrawableComponent>(entity);
        const auto& transform_component =
            m_scene->get_component_manager().get_component<ecs::TransformComponent>(entity);
        if (!drawable_component.bbox.is_valid())
        {
          drawable_component.bbox = utils::calculate_bbox(drawable_component.mesh_nodes);
        }
        const BoundingBox bbox_world = utils::calculate_world_space_bbox(
            drawable_component.bbox, transform_component.transform_node.get_world_mat());
        aabb_bounds.emplace_back(glm::vec4(bbox_world.min(), 1), glm::vec4(bbox_world.max(), 1));
      }
      if (m_scene->get_options().show_scene_bbox)
      {
        const BoundingBox bbox_world = utils::calculate_world_space_bbox(m_scene->get_bbox(), glm::mat4(1.f));
        aabb_bounds.emplace_back(glm::vec4(bbox_world.min(), 1), glm::vec4(bbox_world.max(), 1));
      }
      m_bbox_min_max_points_ssbo.bind();
      m_bbox_min_max_points_ssbo.resize_if_smaller(sizeof(AABBBoundsGPU) * n);
      m_bbox_min_max_points_ssbo.set_data(aabb_bounds.data(), sizeof(AABBBoundsGPU) * aabb_bounds.size(), 0);
      m_bbox_min_max_points_ssbo.unbind();
      m_bbox_min_max_points_ssbo.bind_to_index();
      Shader* shader = ShaderStorage::get(ShaderStorage::ShaderType::BOUNDING_BOX);
      BindChainFIFO bc({ shader, &m_bbox_vao });
      shader->set_vec3("color", glm::vec3(0, 1, 0));
      glDrawArraysInstanced(GL_LINES, 0, 24, n);
    }

    // Normals
    if (!m_normals_vcounts.empty() && !m_normals_voffsets.empty())
    {
      Shader* shader = ShaderStorage::get(ShaderStorage::ShaderType::NORMALS);
      BindChainFIFO bg({ shader, &m_normals_vao });
      shader->set_vec3("normalColor", glm::vec3(0, 1, 1));
      m_scene->get_transforms_ssbo().bind_to_index();
      m_normals_transform_indices_ssbo.bind_to_index();
      glMultiDrawArrays(GL_POINTS, m_normals_voffsets.data(), m_normals_vcounts.data(),
                        static_cast<GLsizei>(m_normals_vcounts.size()));
      shader->unbind();
    }
  }

  void DebugPass::update_normals()
  {
    m_normals_voffsets.clear();
    m_normals_vcounts.clear();

    const auto& entities = m_scene->get_entities_with_visible_normals();
    if (entities.empty())
    {
      return;
    }
    m_normals_voffsets.reserve(entities.size());
    m_normals_vcounts.reserve(entities.size());

    BindChainFIFO bc({ &m_normals_vao, &m_normals_vbo });
    size_t vbo_size = 0;
    std::vector<int32_t> entities_with_visible_normals_ssbo_indices;
    entities_with_visible_normals_ssbo_indices.reserve(entities.size());
    const auto& entity_transform_ssbo_idx_map = m_scene->get_entity_transform_ssbo_idx_map();
    for (EntityId entity : entities)
    {
      const auto& drawable_component = m_scene->get_component_manager().get_component<ecs::DrawableComponent>(entity);
      for (const auto& mesh_node : drawable_component.mesh_nodes)
      {
        vbo_size += mesh_node.mesh.asset->get_vertices().size() * sizeof(Vertex);
      }
      entities_with_visible_normals_ssbo_indices.push_back(entity_transform_ssbo_idx_map.at(entity));
    }
    m_normals_vbo.resize_if_smaller(vbo_size);

    size_t vbo_offset = 0;
    size_t idx = 0;

    for (EntityId entity : entities)
    {
      const auto& drawable_component = m_scene->get_component_manager().get_component<ecs::DrawableComponent>(entity);
      size_t vcount = 0, fcount = 0;
      utils::get_meshes_metadata(drawable_component.mesh_nodes, vcount, fcount);
      m_normals_voffsets.push_back(vbo_offset / sizeof(Vertex));
      m_normals_vcounts.push_back(vcount);
      for (const auto& mesh_node : drawable_component.mesh_nodes)
      {
        m_normals_vbo.set_data(mesh_node.mesh.asset->get_vertices().data(),
                               mesh_node.mesh.asset->get_vertices().size() * sizeof(Vertex), vbo_offset);
        vbo_offset += mesh_node.mesh.asset->get_vertices().size() * sizeof(Vertex);
      }
    }
    m_normals_transform_indices_ssbo.bind();
    m_normals_transform_indices_ssbo.resize_if_smaller(sizeof(int32_t) *
                                                       entities_with_visible_normals_ssbo_indices.size());
    m_normals_transform_indices_ssbo.set_data(entities_with_visible_normals_ssbo_indices.data(),
                                              sizeof(int32_t) * entities_with_visible_normals_ssbo_indices.size(), 0);
    m_normals_transform_indices_ssbo.unbind();
  }

  void DebugPass::add_line(const glm::vec3& a, const glm::vec3& b, const glm::vec4& color)
  {
    m_lines_vertices.emplace_back(a, color);
    m_lines_vertices.emplace_back(b, color);
    m_lines_vbo.bind();
    m_lines_vbo.resize_if_smaller(m_lines_vertices.size() * sizeof(DebugVertex));
    m_lines_vbo.set_data(m_lines_vertices.data(), sizeof(DebugVertex) * m_lines_vertices.size(), 0);
    m_lines_vbo.unbind();
  }

  void DebugPass::clear_lines()
  {
    m_lines_vertices.clear();
  }

  void DebugPass::clear()
  {
    clear_lines();
  }

  void SelectionWheelPass::init(Scene* scene)
  {
    RenderPass::init(scene);
    auto& wheel = scene->get_ui().get_item_selection_wheel();
    const auto& slots = wheel.get_slots();
    if (slots.empty())
    {
      Logger::info("Selection wheel with 0 slots.");
      return;
    }

    const int w = scene->get_window()->width();
    const int h = scene->get_window()->height();
    // arc points + closing lines
    const int num_arc_points = slots[0].arc_segments.size() / 2;
    const int arc_size = 2 * num_arc_points * sizeof(glm::vec2);
    const int total_size = (arc_size * slots.size()) + (slots.size() * 4 * sizeof(glm::vec2));
    m_slots_with_icons.reserve(slots.size());
    m_ortho = glm::ortho(-w / 2.f, w / 2.f, -h / 2.f, h / 2.f);
    m_vao.bind();
    m_vbo.bind();
    m_vao.link_attrib(0, 2, GL_FLOAT, 0, 0);
    m_vbo.resize(total_size);
    std::vector<glm::vec2> slot_closing_lines;
    std::vector<glm::vec2> slot_tri_strip_data(num_arc_points * 2);
    slot_closing_lines.reserve(slots.size() * 4);
    int vbo_bytes_offset = 0;
    for (const auto& slot : slots)
    {
      if (slot.icon)
      {
        m_slots_with_icons.push_back(&slot);
      }
      for (int i = 0; i < num_arc_points; i++)
      {
        // 1 point from inner circle and 1 from outer, so that we can use TRIANGLE_STRIP mode and draw segment with
        // triangles
        slot_tri_strip_data[2 * i] = slot.arc_segments[i];
        slot_tri_strip_data[2 * i + 1] = slot.arc_segments[num_arc_points + i];
      }
      m_vbo.set_data(slot_tri_strip_data.data(), slot_tri_strip_data.size() * sizeof(glm::vec2), vbo_bytes_offset);
      slot_closing_lines.insert(slot_closing_lines.end(), slot.arcs_closing_lines, slot.arcs_closing_lines + 4);
      vbo_bytes_offset += slot_tri_strip_data.size() * sizeof(glm::vec2);
    }
    m_vbo.set_data(slot_closing_lines.data(), slot_closing_lines.size() * sizeof(glm::vec2), vbo_bytes_offset);
    m_vbo.unbind();
    m_vao.unbind();

    struct IconVertex
    {
      glm::vec2 pos;
      glm::vec2 pad;
      glm::vec2 uv;
      glm::vec2 pad2;
    };

    m_vao_icons.bind();
    m_vbo_icons.bind();
    m_vao_icons.link_attrib(0, 2, GL_FLOAT, sizeof(IconVertex), nullptr);
    m_vao_icons.link_attrib(1, 2, GL_FLOAT, sizeof(IconVertex), reinterpret_cast<void*>(offsetof(IconVertex, uv)));
    m_vbo_icons.resize(m_slots_with_icons.size() * sizeof(IconVertex) * 6); // 6 vertices to render texture quad
    for (int i = 0; i < m_slots_with_icons.size(); i++)
    {
      const SelectionWheelSlot& slot = *m_slots_with_icons[i];
      IconVertex icon_data[6] = {};
      icon_data[0].uv = { 0, 0 };
      icon_data[0].pos = slot.center - slot.icon_size;
      icon_data[1].uv = { 1, 0 };
      icon_data[1].pos = { slot.center.x + slot.icon_size, slot.center.y - slot.icon_size };
      icon_data[2].uv = { 0, 1 };
      icon_data[2].pos = { slot.center.x - slot.icon_size, slot.center.y + slot.icon_size };
      icon_data[3].uv = { 0, 1 };
      icon_data[3].pos = { slot.center.x - slot.icon_size, slot.center.y + slot.icon_size };
      icon_data[4].uv = { 1, 0 };
      icon_data[4].pos = { slot.center.x + slot.icon_size, slot.center.y - slot.icon_size };
      icon_data[5].uv = { 1, 1 };
      icon_data[5].pos = slot.center + slot.icon_size;
      m_vbo.set_data(icon_data, sizeof(icon_data), i * sizeof(icon_data));
    }
    m_vbo_icons.unbind();
    m_vao_icons.unbind();
  }

  void SelectionWheelPass::update()
  {
  }

  void SelectionWheelPass::tick(float dt)
  {
    ItemSelectionWheel& wheel = m_scene->get_ui().get_item_selection_wheel();
    if (!wheel.is_visible())
    {
      return;
    }
    Shader* shader = ShaderStorage::get(ShaderStorage::ShaderType::SELECTION_WHEEL);
    shader->bind();
    shader->set_vec3("color", glm::vec3(1, 0, 0));
    shader->set_matrix4f("projectionMatrix", m_ortho);
    m_vao.bind();
    glDisable(GL_DEPTH_TEST);
    const int n_arc_points = wheel.get_slots()[0].arc_segments.size() / 2;
    SelectionWheelSlot* selected_slot = wheel.get_selected_slot();
    for (int i = 0; i < wheel.get_slots().size(); i++)
    {
      shader->set_bool("isSlotSelected", selected_slot == wheel.get_slot(i));
      glDrawArrays(GL_TRIANGLE_STRIP, i * n_arc_points * 2, n_arc_points * 2);
    }
    // set it to false, because if the last slot is selected, it makes closing lines off all segments "selected"
    shader->set_bool("isSlotSelected", false);
    shader->set_vec3("color", glm::vec3(0, 0, 0));
    glLineWidth(5);
    // draw edges of inner and outer arcs
    glDrawArrays(GL_LINES, n_arc_points * 2 * wheel.get_slots().size(), wheel.get_slots().size() * 4);
    m_vao.unbind();
    shader->unbind();

    // render slots icons
    if (!m_slots_with_icons.empty())
    {
      shader = ShaderStorage::get(ShaderStorage::ShaderType::SELECTION_WHEEL_ICON);
      shader->bind();
      shader->set_matrix4f("projectionMatrix", m_ortho);
      m_vao_icons.bind();
      glActiveTexture(GL_TEXTURE0);
      for (int i = 0; i < m_slots_with_icons.size(); i++)
      {
        m_slots_with_icons[i]->icon.asset->bind();
        glDrawArrays(GL_TRIANGLES, i * 6, 6);
        m_slots_with_icons[i]->icon.asset->unbind();
      }
    }
    glEnable(GL_DEPTH_TEST);
    glLineWidth(1);
  }

  InfiniteGridPass::InfiniteGridPass(Scene* scene) : RenderPass(scene)
  {
  }

  void InfiniteGridPass::tick(float)
  {
    if (!m_scene->get_options().show_grid)
    {
      return;
    }
    Shader* shader = ShaderStorage::get(ShaderStorage::ShaderType::GRID);
    BindChainFIFO bchain({ shader, &m_vao });
    glDrawArrays(GL_TRIANGLES, 0, 6);
  }

} // namespace fury
