#include "RenderPass.hpp"
#include "ge/Object3D.hpp"
#include "BindGuard.hpp"
#include "Shader.hpp"
#include "ShaderStorage.hpp"
#include "SceneRenderer.hpp"
#include "Event.hpp"

namespace
{
	void set_default_vertex_attributes(VertexArrayObject& vao)
	{
		vao.link_attrib(0, 3, GL_FLOAT, sizeof(Vertex), nullptr);                        // position
		vao.link_attrib(1, 3, GL_FLOAT, sizeof(Vertex), (void*)(sizeof(GLfloat) * 3));   // normal
		vao.link_attrib(2, 4, GL_FLOAT, sizeof(Vertex), (void*)(sizeof(GLfloat) * 6));   // color
		vao.link_attrib(3, 2, GL_FLOAT, sizeof(Vertex), (void*)(sizeof(GLfloat) * 10));  // texture
	}
}

RenderPass::RenderPass(SceneRenderer* scene) : m_scene(scene)
{
}

GeometryPass::GeometryPass(SceneRenderer* scene) : RenderPass(scene)
{
	BindChainFIFO bind_chain({ &m_vao_indices, &m_vbo_indices });
	::set_default_vertex_attributes(m_vao_indices);
	BindChainFIFO bind_chain2({ &m_vao_arrays, &m_vbo_arrays });
	::set_default_vertex_attributes(m_vao_arrays);
  // TODO: remove listener in dctor
  scene->on_new_object_added += new InstanceListener(this, &GeometryPass::on_new_scene_object);
  Ui& ui = scene->get_ui();
  ui.on_object_change += new InstanceListener(this, &GeometryPass::handle_object_change);
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
  for (const Object3D& obj : m_scene->get_drawables())
  {
    ObjectGeometryMetadata meta = obj.get_geometry_metadata();
    if (obj.get_render_config().use_indices)
    {
      vcount_vbo_indices += meta.vert_count_total;
      idx_count += meta.idx_count_total;
    }
    else
    {
      vcount_vbo_arrays += meta.vert_count_total;
    }
  }
  m_vbo_arrays.resize_if_smaller(vcount_vbo_arrays * sizeof(Vertex));
  m_vbo_indices.resize_if_smaller(vcount_vbo_indices * sizeof(Vertex));
  m_ebo.resize_if_smaller(idx_count * sizeof(GLuint));
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
  for (const Object3D& obj : drawables)
  {
    if (obj.get_render_config().use_indices)
    {
      m_objects_indices_rendering_mode.push_back(&obj);
    }
    else
    {
      m_objects_arrays_rendering_mode.push_back(&obj);
    }
  }
}

void GeometryPass::render_scene()
{
  Camera& camera = m_scene->get_camera();
  std::unordered_set<int>& lights_sources = m_scene->get_light_sources();
  assert(lights_sources.size() == 1);
  const Object3D& light_source = m_scene->get_drawables()[*lights_sources.begin()];

  Shader* shader = &ShaderStorage::get(ShaderStorage::ShaderType::DEFAULT);
  shader->bind();
  shader->set_vec3("viewPos", camera.position());
  shader->set_int("defaultTexture", 0);
  shader->set_int("ambientTex", 1);
  shader->set_int("diffuseTex", 2);
  shader->set_int("specularTex", 3);
  // center in world space
  shader->set_vec3("lightPos", light_source.center() + glm::vec3(light_source.model_matrix()[3]));
  shader->set_vec3("lightColor", glm::vec3(1.f));

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

    for (size_t i = 0; i < size; i++)
    {
      const Object3D* obj = ppobj[i];
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

      for (size_t i = 0; i < mesh_count; i++)
      {
        const MeshRenderOffsets& mesh_offsets = meshes_offsets[i];
        const Mesh& mesh = obj->get_mesh(i);

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

  for (Object3D& obj : m_scene->get_drawables())
  {
    if (obj.is_selected() && obj.has_surface())
    {
      const glm::vec3 old_scale = obj.scale();
      obj.scale(glm::vec3(old_scale + 0.05f));
      shader->set_matrix4f("modelMatrix", obj.model_matrix());
      const auto& render_config = obj.get_render_config();
      const size_t mesh_count = obj.mesh_count();
      const std::vector<MeshRenderOffsets>& meshes_offsets = m_render_offsets.at(&obj);
      for (size_t i = 0; i < mesh_count; i++)
      {
        const MeshRenderOffsets& mesh_offsets = meshes_offsets[i];
        const Mesh& mesh = obj.get_mesh(i);
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
      obj.scale(glm::vec3(old_scale));
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
		return;
	}

  allocate_memory_for_buffers();
  split_objects();
  m_render_offsets.clear();

  size_t vbo_indices_offset = 0;
  size_t vbo_arrays_offset = 0;
  size_t ebo_offset = 0;
  size_t basev = 0;

  for (const Object3D& obj : m_scene->get_drawables())
  {
    const size_t mesh_count = obj.mesh_count();
    const auto& render_config = obj.get_render_config();
    size_t model_vertices_within_indices = 0;
    std::vector<MeshRenderOffsets>& meshes_offsets = m_render_offsets[&obj];
    for (size_t i = 0; i < mesh_count; i++)
    {
      MeshRenderOffsets& mesh_offsets = meshes_offsets.emplace_back();
      const Mesh& mesh = obj.get_mesh(i);
      const size_t vsize = mesh.vertices().size() * sizeof(Vertex);
      const size_t idx_size = mesh.faces_as_indices().size() * sizeof(GLuint);
      if (render_config.use_indices)
      {
        BindChainFIFO bc({&m_vao_indices, &m_vbo_indices, &m_ebo});
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
  const auto& drawables = m_scene->get_drawables();
  if (drawables.empty())
  {
    return;
  }
  render_scene();
  render_selected_objects();
}

NormalsPass::NormalsPass(SceneRenderer* scene) : RenderPass(scene)
{
  Ui& ui = scene->get_ui();
  // TODO: remove listener in dctor
  ui.on_visible_normals_button_pressed += new InstanceListener(this, &NormalsPass::handle_visible_normals_toggle);
  ui.on_object_change += new InstanceListener(this, &NormalsPass::handle_object_change);
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

  size_t vbo_size = 0;
  std::for_each(drawables.begin(), drawables.end(), 
    [&](const Object3D& obj)
    { 
      vbo_size += obj.is_normals_visible() * obj.get_geometry_metadata().vert_count_total * sizeof(Vertex);
    });
  m_vbo.resize_if_smaller(vbo_size);

  BindChainFIFO bc({ &m_vao, &m_vbo });
  size_t vbo_offset = 0;
  size_t idx = 0;
  for (const Object3D& obj : drawables)
  {
    if (!obj.is_normals_visible())
      continue;
    m_objects_with_visible_normals.insert(&obj);
    m_model_matrices.emplace_back(obj.model_matrix());
    const ObjectGeometryMetadata& meta = obj.get_geometry_metadata();
    ObjectRenderOffsets& obj_offsets = m_object_offsets[&obj];
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
  m_model_matrices_ssbo.resize_if_smaller(sizeof(glm::mat4) * m_model_matrices.size());
  m_model_matrices_ssbo.bind();
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

LinesPass::LinesPass(SceneRenderer* scene) : RenderPass(scene)
{
  Ui& ui = scene->get_ui();
  // TODO: remove listener in dctor
  ui.on_visible_bbox_button_pressed += new InstanceListener(this, &LinesPass::handle_visible_bbox_toggle);
  m_ebo.resize(BoundingBox::lines_indices().size() * sizeof(GLuint));
  BindChainFIFO bc({ &m_vao, &m_vbo, &m_ebo });
  ::set_default_vertex_attributes(m_vao);
  m_ebo.set_data(BoundingBox::lines_indices().data(), sizeof(GLuint) * BoundingBox::lines_indices().size(), 0);
}

void LinesPass::update()
{
  m_objects_with_visible_bboxes.clear();
  m_objects_with_visible_bboxes.reserve(m_scene->get_drawables().size());
  constexpr int bbox_vsize_bytes = sizeof(Vertex) * 8;
  for (const Object3D& drawable : m_scene->get_drawables())
  {
    if (drawable.is_bbox_visible())
      m_objects_with_visible_bboxes.push_back(&drawable);
  }
  // 8 vertices for each bbox
  m_vbo.resize_if_smaller(m_objects_with_visible_bboxes.size() * bbox_vsize_bytes);

  BindGuard bg(m_vbo);
  for (size_t i = 0; i < m_objects_with_visible_bboxes.size(); i++)
  {
    const Object3D* drawable = m_objects_with_visible_bboxes[i];
    const std::vector<Vertex>& points = drawable->bbox().points();
    m_vbo.set_data(points.data(), bbox_vsize_bytes, i * bbox_vsize_bytes);
  }
}

void LinesPass::tick()
{
  if (m_objects_with_visible_bboxes.empty())
    return;
  Shader* shader = &ShaderStorage::get(ShaderStorage::ShaderType::LINES);
  BindGuard bg_shader(shader);
  BindGuard bg_vao(m_vao);
  shader->set_vec3("lineColor", glm::vec3(0, 1, 0));
  for (size_t i = 0; i < m_objects_with_visible_bboxes.size(); i++)
  {
    const Object3D* drawable = m_objects_with_visible_bboxes[i];
    shader->set_matrix4f("modelMatrix", drawable->model_matrix());
    glDrawElementsBaseVertex(GL_LINES, 24, GL_UNSIGNED_INT, 0, i * 8);
  }
}

void LinesPass::handle_visible_bbox_toggle(Object3D* obj, bool is_visible)
{
  // TODO: better impl
  update();
}
