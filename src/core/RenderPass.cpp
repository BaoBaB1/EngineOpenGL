#include "RenderPass.hpp"
#include "ge/Object3D.hpp"
#include "ge/ItemSelectionWheel.hpp"
#include "WindowGLFW.hpp"
#include "ObjectChangeInfo.hpp"
#include "BindGuard.hpp"
#include "Shader.hpp"
#include "ShaderStorage.hpp"
#include "SceneRenderer.hpp"
#include "Event.hpp"
#include "ge/Polyline.hpp"

namespace
{
	void set_default_vertex_attributes(fury::VertexArrayObject& vao)
	{
		vao.link_attrib(0, 3, GL_FLOAT, sizeof(fury::Vertex), nullptr);                        // position
		vao.link_attrib(1, 3, GL_FLOAT, sizeof(fury::Vertex), (void*)(sizeof(GLfloat) * 3));   // normal
		vao.link_attrib(2, 4, GL_FLOAT, sizeof(fury::Vertex), (void*)(sizeof(GLfloat) * 6));   // color
		vao.link_attrib(3, 2, GL_FLOAT, sizeof(fury::Vertex), (void*)(sizeof(GLfloat) * 10));  // texture
	}
}

namespace fury
{
  RenderPass::RenderPass(SceneRenderer* scene) : m_scene(scene)
  {
  }

  GeometryPass::GeometryPass(SceneRenderer* scene, int shadow_map_texture) : RenderPass(scene)
  {
    m_shadow_map_texture = shadow_map_texture;
    BindChainFIFO bind_chain({ &m_vao_indices, &m_vbo_indices });
    ::set_default_vertex_attributes(m_vao_indices);
    BindChainFIFO bind_chain2({ &m_vao_arrays, &m_vbo_arrays });
    ::set_default_vertex_attributes(m_vao_arrays);
    // TODO: remove listener in dctor
    scene->on_new_object_added += new InstanceListener(this, &GeometryPass::on_new_scene_object);
    SceneInfo* scene_info_component = scene->get_ui().get_component<SceneInfo>("SceneInfo");
    scene_info_component->on_object_change += new InstanceListener(this, &GeometryPass::handle_object_change);
  }

  void GeometryPass::on_new_scene_object(Object3D* obj)
  {
    update();
  }

  void GeometryPass::handle_object_change(Object3D* obj, const ObjectChangeInfo& info)
  {
    // color change
    if (info.is_vertex_change)
    {
      std::vector<MeshRenderOffsets>& offsets = m_render_offsets.at(obj);
      BindGuard bg(obj->get_render_config().use_indices ? &m_vbo_indices : &m_vbo_arrays);
      for (const MeshRenderOffsets& mesh_offset : offsets)
      {
        const size_t mesh_count = obj->mesh_count();
        for (size_t i = 0; i < mesh_count; i++)
        {
          const Mesh& mesh = obj->get_mesh(i);
          if (obj->get_render_config().use_indices)
          {
            m_vbo_indices.set_data(mesh.vertices().data(), mesh.vertices().size() * sizeof(Vertex), mesh_offset.vbo_indices_offset);
          }
          else
          {
            m_vbo_arrays.set_data(mesh.vertices().data(), mesh.vertices().size() * sizeof(Vertex), mesh_offset.vbo_arrays_offset);
          }
        }
      }
    }
    else if (info.is_shading_mode_change)
    {
      update();
    }
  }

  void GeometryPass::allocate_memory_for_buffers()
  {
    size_t vcount_vbo_indices = 0;
    size_t vcount_vbo_arrays = 0;
    size_t idx_count = 0;
    for (const auto& obj : m_scene->get_drawables())
    {
      ObjectGeometryMetadata meta = obj->get_geometry_metadata();
      if (obj->get_render_config().use_indices)
      {
        assert(obj->get_render_config().mode == GL_TRIANGLES);
        vcount_vbo_indices += meta.vert_count_total;
        idx_count += meta.face_count_total * 3;
      }
      else
      {
        vcount_vbo_arrays += meta.vert_count_total;
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
    const auto& drawables = m_scene->get_drawables();
    m_objects_indices_rendering_mode.reserve(drawables.size());
    m_objects_arrays_rendering_mode.reserve(drawables.size());
    for (const auto& obj : drawables)
    {
      if (obj->get_render_config().use_indices)
      {
        m_objects_indices_rendering_mode.push_back(obj.get());
      }
      else
      {
        m_objects_arrays_rendering_mode.push_back(obj.get());
      }
    }
  }

  void GeometryPass::render_scene()
  {
    Camera& camera = m_scene->get_camera();
    Shader* shader = &ShaderStorage::get(ShaderStorage::ShaderType::DEFAULT);
    const DirectionalLight& dir_light = m_scene->get_directional_light();
    shader->bind();
    shader->set_vec3("viewPos", camera.position());
    shader->set_int("defaultTexture", 0);
    shader->set_int("ambientTex", 1);
    shader->set_int("diffuseTex", 2);
    shader->set_int("specularTex", 3);
    shader->set_int("shadowMap", 4);
    shader->set_matrix4f("lightSpaceVPMatrix", dir_light.proj_matrix * dir_light.view_matrix);
    shader->set_vec3("lightDirGlobal", dir_light.direction);
    shader->set_vec3("lightPos", glm::mat4(1.f) * glm::vec4(dir_light.position, 1));
    shader->set_vec3("lightColor", glm::vec3(1.f));

    // set shadow map
    glActiveTexture(GL_TEXTURE0 + 4);
    glBindTexture(GL_TEXTURE_2D, m_shadow_map_texture);

    for (int i = 0; i < 2; i++)
    {
      const Object3D** ppobj = nullptr;
      size_t size = 0;
      if (i == 0)
      {
        if (m_objects_indices_rendering_mode.empty())
          continue;
        m_vao_indices.bind();
        size = m_objects_indices_rendering_mode.size();
        ppobj = m_objects_indices_rendering_mode.data();
      }
      else
      {
        if (m_objects_arrays_rendering_mode.empty())
          continue;
        m_vao_arrays.bind();
        size = m_objects_arrays_rendering_mode.size();
        ppobj = m_objects_arrays_rendering_mode.data();
      }

      for (size_t obj_i = 0; obj_i < size; obj_i++)
      {
        const Object3D* obj = ppobj[obj_i];
        shader->set_matrix4f("modelMatrix", obj->model_matrix());
        shader->set_bool("applyShading", obj->shading_mode() != Object3D::ShadingMode::NO_SHADING && !obj->is_light_source());

        if (obj->is_selected() && obj->has_surface())
        {
          // enable writing to the stencil buffer and disable it in the end of current iteration
          glStencilFunc(GL_ALWAYS, 1, 0xFF);
          glStencilMask(0xFF);
        }

        const auto& render_config = obj->get_render_config();
        const size_t mesh_count = obj->mesh_count();
        const std::vector<MeshRenderOffsets>& meshes_offsets = m_render_offsets.at(obj);

        for (size_t mesh_i = 0; mesh_i < mesh_count; mesh_i++)
        {
          const MeshRenderOffsets& mesh_offsets = meshes_offsets[mesh_i];
          const Mesh& mesh = obj->get_mesh(mesh_i);

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

          // set mesh material
          const auto& mat = mesh.material();
          shader->set_vec3("material.ambient", mat.ambient);
          shader->set_vec3("material.diffuse", mat.diffuse);
          shader->set_vec3("material.specular", mat.specular);
          shader->set_float("material.shininess", mat.shininess);
          shader->set_float("material.alpha", mat.alpha);

          if (render_config.use_indices)
          {
            glDrawElementsBaseVertex(render_config.mode, mesh.faces_as_indices().size(), GL_UNSIGNED_INT, (void*)mesh_offsets.ebo_offset, mesh_offsets.basev);
          }
          else
          {
            glDrawArrays(render_config.mode, static_cast<GLint>(mesh_offsets.vbo_arrays_offset), static_cast<GLsizei>(mesh.vertices().size()));
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
    }
    m_vao_indices.unbind();
    m_vao_arrays.unbind();
    shader->unbind();
  }

  void GeometryPass::render_selected_objects()
  {
    Shader* shader = &ShaderStorage::get(ShaderStorage::ShaderType::OUTLINING);
    shader->bind();

    //glDisable(GL_DEPTH_TEST);
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    glStencilMask(0x00);

    // TODO: think about better implementation.
    // we can store selected drawables inside split_objects() function, but then there is no convenient way to update them.
    // calling update() each time for that is wasteful since it updates many internal states.

    for (Object3D* obj : m_scene->get_selected_objects())
    {
      if (obj->has_surface())
      {
        const glm::vec3 old_scale = obj->scale();
        obj->scale(glm::vec3(old_scale + 0.05f));
        shader->set_matrix4f("modelMatrix", obj->model_matrix());
        const auto& render_config = obj->get_render_config();
        const size_t mesh_count = obj->mesh_count();
        const std::vector<MeshRenderOffsets>& meshes_offsets = m_render_offsets.at(obj);
        for (size_t i = 0; i < mesh_count; i++)
        {
          const MeshRenderOffsets& mesh_offsets = meshes_offsets[i];
          const Mesh& mesh = obj->get_mesh(i);
          if (render_config.use_indices)
          {
            BindGuard bg(m_vao_indices);
            glDrawElementsBaseVertex(render_config.mode, mesh.faces_as_indices().size(), GL_UNSIGNED_INT, (void*)mesh_offsets.ebo_offset, mesh_offsets.basev);
          }
          else
          {
            BindGuard bg(m_vao_arrays);
            glDrawArrays(render_config.mode, static_cast<GLint>(mesh_offsets.vbo_arrays_offset), static_cast<GLsizei>(mesh.vertices().size()));
          }
        }
        obj->scale(glm::vec3(old_scale));
      }
    }
    glStencilMask(0xFF);
    glStencilFunc(GL_ALWAYS, 0, 0xFF);
    glEnable(GL_DEPTH_TEST);
    shader->unbind();
  }

  void GeometryPass::update()
  {
    if (m_scene->get_drawables().empty())
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

    for (const auto& obj : m_scene->get_drawables())
    {
      const size_t mesh_count = obj->mesh_count();
      const auto& render_config = obj->get_render_config();
      size_t model_vertices_within_indices = 0;
      std::vector<MeshRenderOffsets>& meshes_offsets = m_render_offsets[obj.get()];
      for (size_t i = 0; i < mesh_count; i++)
      {
        MeshRenderOffsets& mesh_offsets = meshes_offsets.emplace_back();
        const Mesh& mesh = obj->get_mesh(i);
        const size_t vsize = mesh.vertices().size() * sizeof(Vertex);
        const size_t idx_size = mesh.faces_as_indices().size() * sizeof(GLuint);
        if (render_config.use_indices)
        {
          BindChainFIFO bc({ &m_vao_indices, &m_vbo_indices, &m_ebo });
          m_vbo_indices.set_data(mesh.vertices().data(), vsize, vbo_indices_offset);
          m_ebo.set_data(mesh.faces_as_indices().data(), idx_size, ebo_offset);
          mesh_offsets.vbo_indices_offset = vbo_indices_offset;
          mesh_offsets.ebo_offset = ebo_offset;
          mesh_offsets.basev = basev;
          basev += mesh.vertices().size();
          vbo_indices_offset += vsize;
          ebo_offset += idx_size;
        }
        else
        {
          BindChainFIFO bc({ &m_vao_arrays, &m_vbo_arrays });
          m_vbo_arrays.set_data(mesh.vertices().data(), vsize, vbo_arrays_offset);
          // index offset (not in bytes)
          mesh_offsets.vbo_arrays_offset = vbo_arrays_offset / sizeof(Vertex);
          vbo_arrays_offset += vsize;
        }
      }
    }
  }

  void GeometryPass::tick()
  {
    if (m_scene->get_drawables().empty())
    {
      return;
    }
    render_scene();
    render_selected_objects();
  }

  NormalsPass::NormalsPass(SceneRenderer* scene) : RenderPass(scene)
  {
    // TODO: remove listener in dctor
    SceneInfo* scene_info_component = scene->get_ui().get_component<SceneInfo>("SceneInfo");
    Gizmo* gizmo_component = scene->get_ui().get_component<Gizmo>("Gizmo");
    scene_info_component->on_visible_normals_button_pressed += new InstanceListener(this, &NormalsPass::handle_visible_normals_toggle);
    scene_info_component->on_object_change += new InstanceListener(this, &NormalsPass::handle_object_change);
    gizmo_component->on_object_change += new InstanceListener(this, &NormalsPass::handle_object_change);
    BindChainFIFO bc({ &m_vao, &m_vbo });
    ::set_default_vertex_attributes(m_vao);
    m_model_matrices_ssbo.set_binding_point(1);
  }

  void NormalsPass::update()
  {
    m_model_matrices.clear();
    m_voffsets.clear();
    m_vcounts.clear();
    m_objects_with_visible_normals.clear();
    m_object_offsets.clear();

    const auto& drawables = m_scene->get_drawables();
    m_model_matrices.reserve(drawables.size());
    m_voffsets.reserve(drawables.size());
    m_vcounts.reserve(drawables.size());

    BindChainFIFO bc({ &m_vao, &m_vbo });

    size_t vbo_size = 0;
    std::for_each(drawables.begin(), drawables.end(),
      [&](const std::unique_ptr<Object3D>& obj)
      {
        vbo_size += obj->is_normals_visible() * obj->get_geometry_metadata().vert_count_total * sizeof(Vertex);
      });
    m_vbo.resize_if_smaller(vbo_size);

    size_t vbo_offset = 0;
    size_t idx = 0;
    for (const auto& obj : drawables)
    {
      if (!obj->is_normals_visible())
        continue;
      m_objects_with_visible_normals.insert(obj.get());
      m_model_matrices.emplace_back(obj->model_matrix());
      ObjectGeometryMetadata meta = obj->get_geometry_metadata();
      ObjectRenderOffsets& obj_offsets = m_object_offsets[obj.get()];
      m_voffsets.push_back(vbo_offset / sizeof(Vertex));
      m_vcounts.push_back(meta.vert_count_total);
      obj_offsets.vcount = meta.vert_count_total;
      obj_offsets.vbo_offset_vtx_count = vbo_offset / sizeof(Vertex);
      obj_offsets.internal_idx = idx++;
      for (const MeshGeometryMetadata& mesh_meta : meta.meshes_data)
      {
        m_vbo.set_data(mesh_meta.vdata, mesh_meta.vert_count * sizeof(Vertex), vbo_offset);
        vbo_offset += mesh_meta.vert_count * sizeof(Vertex);
      }
    }
    m_model_matrices_ssbo.bind();
    m_model_matrices_ssbo.resize_if_smaller(sizeof(glm::mat4) * m_model_matrices.size());
    m_model_matrices_ssbo.set_data(m_model_matrices.data(), sizeof(glm::mat4) * m_model_matrices.size(), 0);
    m_model_matrices_ssbo.unbind();
  }

  void NormalsPass::tick()
  {
    if (m_objects_with_visible_normals.empty())
      return;
    Shader* shader = &ShaderStorage::get(ShaderStorage::ShaderType::NORMALS);
    shader->bind();
    shader->set_vec3("normalColor", glm::vec3(0, 1, 1));
    BindGuard bg(m_vao);
    glMultiDrawArrays(GL_POINTS, m_voffsets.data(), m_vcounts.data(), static_cast<GLsizei>(m_objects_with_visible_normals.size()));
    shader->unbind();
  }

  void NormalsPass::handle_visible_normals_toggle(Object3D* obj, bool is_visible)
  {
    // if we don't have info about this object yet
    if (m_object_offsets.count(obj) == 0)
    {
      update();
    }
    else
    {
      // just add/exclude it from rendering since we already have info about it
      m_voffsets.clear();
      m_vcounts.clear();
      m_model_matrices.clear();

      if (is_visible)
      {
        m_objects_with_visible_normals.insert(obj);
      }
      else
      {
        m_objects_with_visible_normals.erase(obj);
      }

      size_t idx = 0;
      for (const Object3D* obj : m_objects_with_visible_normals)
      {
        ObjectRenderOffsets& obj_info = m_object_offsets[obj];
        m_voffsets.push_back(obj_info.vbo_offset_vtx_count);
        m_vcounts.push_back(obj_info.vcount);
        m_model_matrices.push_back(obj->model_matrix());
        obj_info.internal_idx = idx++;
      }
      m_model_matrices_ssbo.bind();
      m_model_matrices_ssbo.set_data(m_model_matrices.data(), sizeof(glm::mat4) * m_model_matrices.size(), 0);
      m_model_matrices_ssbo.unbind();
    }
  }

  void NormalsPass::handle_object_change(Object3D* obj, const ObjectChangeInfo& info)
  {
    // if we have already rendered normals for this object
    if (m_object_offsets.count(obj) != 0)
    {
      // if we moved object 
      if (info.is_transformation_change)
      {
        // update model matrix
        ObjectRenderOffsets& info = m_object_offsets.at(obj);
        m_model_matrices_ssbo.bind();
        m_model_matrices_ssbo.set_data(glm::value_ptr(obj->model_matrix()), sizeof(glm::mat4), info.internal_idx * sizeof(glm::mat4));
        m_model_matrices_ssbo.unbind();
      }
      // shading mode has changed
      else if (info.is_shading_mode_change)
      {
        update();
      }
    }
  }

  BoundingBoxPass::BoundingBoxPass(SceneRenderer* scene) : RenderPass(scene)
  {
    SceneInfo* scene_info_component = scene->get_ui().get_component<SceneInfo>("SceneInfo");
    Gizmo* gizmo_component = scene->get_ui().get_component<Gizmo>("Gizmo");
    // TODO: remove listener in dctor
    scene_info_component->on_visible_bbox_button_pressed += new InstanceListener(this, &BoundingBoxPass::handle_visible_bbox_toggle);
    scene_info_component->on_show_scene_bbox += new InstanceListener(this, &BoundingBoxPass::handle_scene_visible_bbox_toggle);
    scene_info_component->on_object_change += new InstanceListener(this, &BoundingBoxPass::handle_object_change);
    gizmo_component->on_object_change += new InstanceListener(this, &BoundingBoxPass::handle_object_change);

    constexpr static GLuint cube_lines_indices[24] = {
      0, 1, 1, 2, 2, 3, 3, 0, // front
      4, 5, 5, 6, 6, 7, 7, 4, // back
      0, 4, 3, 7, 1, 5, 2, 6
    };

    constexpr static float unit_cube[24] =
    {
      -0.5, -0.5, 0.5, 0.5, -0.5, 0.5, 0.5, 0.5, 0.5, -0.5, 0.5, 0.5,
      -0.5, -0.5, -0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, -0.5
    };

    m_vao.bind();
    m_vbo.bind();
    m_vao.link_attrib(0, 3, GL_FLOAT, 3 * sizeof(float), 0);
    m_vbo.resize(sizeof(unit_cube));
    m_vbo.set_data(&unit_cube, sizeof(unit_cube), 0);
    m_ebo.bind();
    m_ebo.resize(sizeof(cube_lines_indices));
    m_ebo.set_data(&cube_lines_indices, sizeof(cube_lines_indices), 0);
    m_vbo.unbind();
    m_vbo_instance.bind();
    for (int i = 0; i < 4; i++)
    {
      m_vao.link_attrib(i + 1, 4, GL_FLOAT, sizeof(glm::mat4), (void*)(sizeof(glm::vec4) * i));
      glVertexAttribDivisor(i + 1, 1);
    }
    m_vao.unbind();
  }

  void BoundingBoxPass::update()
  {
    std::vector<glm::mat4> instance_data;
    instance_data.reserve(m_scene->get_drawables().size() + 1);
    for (const auto& drawable : m_scene->get_drawables())
    {
      if (drawable->is_bbox_visible())
      {
        const glm::mat4& model_mat = drawable->model_matrix();
        const glm::vec3 center_world = model_mat * glm::vec4(drawable->bbox().center(), 1);
        const glm::vec3 min_world = model_mat * glm::vec4(drawable->bbox().min(), 1);
        const glm::vec3 max_world = model_mat * glm::vec4(drawable->bbox().max(), 1);
        glm::mat4& out_mat = instance_data.emplace_back(1.f);
        out_mat = glm::translate(out_mat, center_world);
        out_mat = glm::scale(out_mat, max_world - min_world);
      }
    }
    if (m_is_scene_bbox_visible)
    {
      glm::mat4& mat = instance_data.emplace_back(1.f);
      // scene's bbox is already in world space
      mat = glm::translate(mat, m_scene->get_bbox().center());
      mat = glm::scale(mat, m_scene->get_bbox().max() - m_scene->get_bbox().min());
    }
    m_instances = instance_data.size();
    m_vbo_instance.bind();
    m_vbo_instance.resize_if_smaller(instance_data.size() * sizeof(glm::mat4));
    m_vbo_instance.set_data(instance_data.data(), m_vbo_instance.get_size(), 0);
    m_vbo_instance.unbind();
  }

  void BoundingBoxPass::tick()
  {
    if (m_instances == 0)
      return;
    Shader* shader = &ShaderStorage::get(ShaderStorage::ShaderType::BOUNDING_BOX);
    BindGuard bg_shader(shader);
    BindGuard bg_vao(m_vao);
    shader->set_vec3("color", glm::vec3(0, 1, 0));
    glDrawElementsInstanced(GL_LINES, 24, GL_UNSIGNED_INT, 0, m_instances);
  }

  void BoundingBoxPass::handle_visible_bbox_toggle(Object3D* obj, bool is_visible)
  {
    // TODO: better impl
    update();
  }

  void BoundingBoxPass::handle_scene_visible_bbox_toggle(bool is_visible)
  {
    m_is_scene_bbox_visible = is_visible;
    update();
  }

  void BoundingBoxPass::handle_object_change(Object3D* obj, const ObjectChangeInfo& info)
  {
    if (info.is_transformation_change)
    {
      update();
    }
  }

  ShadowsPass::ShadowsPass(SceneRenderer* scene, GeometryPass* gp) : RenderPass(scene)
  {
    m_gp = gp;
  }

  void ShadowsPass::update()
  {
    Shader* shader = &ShaderStorage::get(ShaderStorage::ShaderType::SHADOW_MAP);
    shader->bind();
    shader->set_matrix4f("lightViewMatrix", m_scene->get_directional_light().view_matrix);
    shader->set_matrix4f("lightProjectionMatrix", m_scene->get_directional_light().proj_matrix);
    for (int i = 0; i < 2; i++)
    {
      const Object3D** ppobj = nullptr;
      size_t size = 0;
      if (i == 0)
      {
        if (m_gp->m_objects_indices_rendering_mode.empty())
          continue;
        m_gp->m_vao_indices.bind();
        size = m_gp->m_objects_indices_rendering_mode.size();
        ppobj = m_gp->m_objects_indices_rendering_mode.data();
      }
      else
      {
        if (m_gp->m_objects_arrays_rendering_mode.empty())
          continue;
        m_gp->m_vao_arrays.bind();
        size = m_gp->m_objects_arrays_rendering_mode.size();
        ppobj = m_gp->m_objects_arrays_rendering_mode.data();
      }

      for (size_t obj_i = 0; obj_i < size; obj_i++)
      {
        const Object3D* obj = ppobj[obj_i];
        shader->set_matrix4f("modelMatrix", obj->model_matrix());

        const auto& render_config = obj->get_render_config();
        const size_t mesh_count = obj->mesh_count();
        const std::vector<GeometryPass::MeshRenderOffsets>& meshes_offsets = m_gp->m_render_offsets.at(obj);

        for (size_t mesh_i = 0; mesh_i < mesh_count; mesh_i++)
        {
          const GeometryPass::MeshRenderOffsets& mesh_offsets = meshes_offsets[mesh_i];
          const Mesh& mesh = obj->get_mesh(mesh_i);
          if (render_config.use_indices)
          {
            glDrawElementsBaseVertex(render_config.mode, mesh.faces_as_indices().size(), GL_UNSIGNED_INT, (void*)mesh_offsets.ebo_offset, mesh_offsets.basev);
          }
          else
          {
            glDrawArrays(render_config.mode, static_cast<GLint>(mesh_offsets.vbo_arrays_offset), static_cast<GLsizei>(mesh.vertices().size()));
          }
        }
      }
    }
    m_gp->m_vao_indices.unbind();
    m_gp->m_vao_arrays.unbind();
    shader->unbind();
  }

  void ShadowsPass::tick()
  {
  }

  DebugPass::DebugPass(SceneRenderer* scene) : RenderPass(scene)
  {
    BindChainFIFO bc({ &m_vao, &m_vbo });
    ::set_default_vertex_attributes(m_vao);
  }

  void DebugPass::update()
  {
    size_t poly_vsize = 0;
    std::for_each(m_polys.begin(), m_polys.end(), 
      [&](const Polyline& poly) { poly_vsize += poly.get_points().size() * sizeof(Vertex); });
    BindGuard bg(m_vbo);
    m_vbo.resize_if_smaller(poly_vsize);
    size_t offset = 0;
    for (const Polyline& poly : m_polys)
    {
      m_vbo.set_data(poly.get_points().data(), poly.get_points().size() * sizeof(Vertex), offset);
      offset += poly.get_points().size() * sizeof(Vertex);
    }
  }

  void DebugPass::tick()
  {
    if (m_polys.empty())
      return;
    Shader& shader = ShaderStorage::get(ShaderStorage::ShaderType::SIMPLE);
    shader.bind();
    shader.set_vec3("color", glm::vec3(0, 1, 0));
    m_vao.bind();
    size_t first = 0;
    for (const Polyline& poly : m_polys)
    {
      shader.set_matrix4f("modelMatrix", glm::mat4(1.f));
      glDrawArrays(GL_LINE_STRIP, first, poly.get_points().size());
      first += poly.get_points().size();
    }
    m_vao.unbind();
    shader.unbind();
  }

  void DebugPass::add_poly(const Polyline& poly)
  {
    m_polys.push_back(poly);
  }

  void DebugPass::clear()
  {
    m_polys.clear();
  }

  SelectionWheelPass::SelectionWheelPass(SceneRenderer* scene, ItemSelectionWheel* wheel) : RenderPass(scene), m_wheel(wheel)
  {
    const auto& slots = wheel->get_slots();
    if (slots.empty())
    {
      Logger::info("Selection wheel with 0 slots.");
      return;
    }

    const int w = scene->get_window()->width();
    const int h = scene->get_window()->height();
    // arc points + closing lines
    const int num_arc_points = slots[0].arcs_data.size() / 2;
    const int arc_size = 2 * num_arc_points * sizeof(glm::vec2);
    const int total_size = (arc_size * slots.size()) + (slots.size() * 4 * sizeof(glm::vec2));
    m_slots_with_icons.reserve(slots.size());
    m_ortho = glm::ortho<float>(-w / 2, w / 2, -h / 2, h / 2);
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
        m_slots_with_icons.push_back(&slot);
      for (int i = 0; i < num_arc_points; i++)
      {
        // 1 point from inner circle and 1 from outer, so that we can use TRIANGLE_STRIP mode and draw segment with triangles
        slot_tri_strip_data[2 * i] = slot.arcs_data[i];
        slot_tri_strip_data[2 * i + 1] = slot.arcs_data[num_arc_points + i];
      }
      m_vbo.set_data(slot_tri_strip_data.data(), slot_tri_strip_data.size() * sizeof(glm::vec2), vbo_bytes_offset);
      slot_closing_lines.insert(slot_closing_lines.end(), slot.arcs_closing_lines_data, slot.arcs_closing_lines_data + 4);
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

  void SelectionWheelPass::tick()
  {
    if (!m_wheel->is_visible())
      return;
    Shader* shader = &ShaderStorage::get(ShaderStorage::ShaderType::SELECTION_WHEEL);
    shader->bind();
    shader->set_vec3("color", glm::vec3(1, 0, 0));
    shader->set_matrix4f("projectionMatrix", m_ortho);
    m_vao.bind();
    glDisable(GL_DEPTH_TEST);
    const int n_arc_points = m_wheel->get_slots()[0].arcs_data.size() / 2;
    SelectionWheelSlot* selected_slot = m_wheel->get_selected_slot();
    for (int i = 0; i < m_wheel->get_slots().size(); i++)
    {
      shader->set_bool("isSlotSelected", selected_slot == m_wheel->get_slot(i));
      glDrawArrays(GL_TRIANGLE_STRIP, i * n_arc_points * 2, n_arc_points * 2);
    }
    // set it to false, because if the last slot is selected, it makes closing lines off all segments "selected"
    shader->set_bool("isSlotSelected", false);
    shader->set_vec3("color", glm::vec3(0, 0, 0));
    glLineWidth(5);
    // draw edges of inner and outer arcs
    glDrawArrays(GL_LINES, n_arc_points * 2 * m_wheel->get_slots().size(), m_wheel->get_slots().size() * 4);
    m_vao.unbind();
    shader->unbind();

    // render slots icons
    if (!m_slots_with_icons.empty())
    {
      shader = &ShaderStorage::get(ShaderStorage::ShaderType::SELECTION_WHEEL_ICON);
      shader->bind();
      shader->set_matrix4f("projectionMatrix", m_ortho);
      m_vao_icons.bind();
      glActiveTexture(GL_TEXTURE0);
      for (int i = 0; i < m_slots_with_icons.size(); i++)
      {
        m_slots_with_icons[i]->icon->bind();
        glDrawArrays(GL_TRIANGLES, i * 6, 6);
        m_slots_with_icons[i]->icon->unbind();
      }
    }
    glEnable(GL_DEPTH_TEST);
    glLineWidth(1);
  }

  InfiniteGridPass::InfiniteGridPass(SceneRenderer* scene) : RenderPass(scene)
  {
  }

  void InfiniteGridPass::tick()
  {
    if (!m_scene->get_ui().get_component<SceneInfo>("SceneInfo")->is_grid_visible())
    {
      return;
    }
    Shader& shader = ShaderStorage::get(ShaderStorage::ShaderType::GRID);
    BindChainFIFO bchain({ &shader, &m_vao });
    glDrawArrays(GL_TRIANGLES, 0, 6);
  }

}
