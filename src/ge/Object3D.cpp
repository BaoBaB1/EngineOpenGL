#include "Object3D.hpp"
#include "core/Logger.hpp"

Object3D::Object3D() : Entity("Object")
{
}

Object3D::Object3D(const std::string& name) : Entity(name)
{
}

void Object3D::rotate(float angle, const glm::vec3& axis)
{
  if (axis == glm::vec3())
    return;
  constexpr float rotation_speed = 10.f;
  m_rotation_angle = angle;
  m_rotation_axis = axis;
  m_model_mat = glm::rotate(m_model_mat, glm::radians(angle * m_delta_time * rotation_speed), glm::normalize(axis));
}

void Object3D::scale(const glm::vec3& scale)
{
  // get rid of current scale factor (https://gamedev.stackexchange.com/questions/119702/fastest-way-to-neutralize-scale-in-the-transform-matrix)
  for (int i = 0; i < 3; i++)
    m_model_mat[i] = glm::normalize(m_model_mat[i]);
  m_model_mat = glm::scale(m_model_mat, scale);
}

void Object3D::translate(const glm::vec3& translation)
{
  m_model_mat = glm::translate(m_model_mat, translation);
}

std::optional<RayHit> Object3D::hit(const Ray& ray) const
{
  if (m_bbox.is_empty())
  {
    const_cast<Object3D*>(this)->calculate_bbox();
  }
  std::optional<RayHit> rhit;
  if (ray.intersect_aabb(m_bbox))
  {
    if (m_render_config.mode == GL_TRIANGLES)
    {
      if (m_render_config.use_indices)
      {
        for (const auto& mesh : *m_meshes)
        {
          for (const auto& face : mesh.faces())
          {
             if (auto hit = ray.intersect_triangle(
                mesh.get_vertex(face[0]).position, mesh.get_vertex(face[1]).position, mesh.get_vertex(face[2]).position)
              )
            {
               // find closest hit
              if (!rhit || rhit->distance > hit->distance)
                rhit = hit;
            }
          }
        }
      }
      else
      {
        for (const auto& mesh : *m_meshes)
        {
          const auto& vertices = mesh.vertices();
          for (size_t i = 0; i < vertices.size(); i += 3)
          {
            if (auto hit = ray.intersect_triangle(
              mesh.get_vertex(i).position, mesh.get_vertex(i + 1).position, mesh.get_vertex(i + 2).position)
              )
            {
              // find closest hit
              if (!rhit || rhit->distance > hit->distance)
                rhit = hit;
            }
          }
        }
      }
    }
    else
    {
      Logger::info("Bounding box is intersected, but could not test if object is actually hit. Primitives are not triangles");
    }
  }
  return rhit;
}

ObjectGeometryMetadata Object3D::get_geometry_metadata() const
{
  ObjectGeometryMetadata res;
  for (const auto& mesh : *m_meshes)
  {
    MeshGeometryMetadata& mesh_data = res.meshes_data.emplace_back();
    res.vert_count_total += mesh.vertices().size();
    mesh_data.vert_count = mesh.vertices().size();
    if (m_render_config.use_indices)
    {
      assert(m_render_config.mode == GL_TRIANGLES);
      res.idx_count_total += mesh.faces().size() * 3;
      mesh_data.idx_count = mesh.faces().size() * 3;
      mesh_data.vdata = mesh.vertices().data();
      mesh_data.idx_data = mesh.faces_as_indices().data();
    }
  }
  return res;
}

void Object3D::add_mesh(Mesh&& mesh)
{
  set_flag(GEOMETRY_MODIFIED, true);
  m_meshes->push_back(std::move(mesh)); 
}

void Object3D::add_mesh(const Mesh& mesh)
{
  set_flag(GEOMETRY_MODIFIED, true);
  m_meshes->push_back(mesh); 
}

void Object3D::calculate_bbox(bool force)
{
  if (!force && !m_bbox.is_empty())
  {
    return;
  }
  glm::vec3 min(INFINITY), max(-INFINITY);
  for (const auto& mesh : *m_meshes)
  {
    for (const Vertex& v : mesh.vertices())
    {
      min = glm::min(min, v.position);
      max = glm::max(max, v.position);
    }
  }
  m_bbox.init(min, max);
}

void Object3D::set_color(const glm::vec4& color)
{
  m_color = color;
  auto apply_color = [](std::vector<Mesh>& meshes, const glm::vec4& color)
    {
      for (auto& mesh : meshes)
      {
        for (auto& vertex : mesh.vertices())
        {
          vertex.color = color;
        }
      }
    };

  // set color of current mesh
  apply_color(*m_meshes, color);

  // set color of cached meshes
  for (auto& cached_meshes : m_cached_meshes)
  {
    if (cached_meshes)
    {
      apply_color(*cached_meshes, color);
    }
  }
}

glm::vec3 Object3D::center() const
{
  if (!get_flag(GEOMETRY_MODIFIED))
  {
    return m_center;
  }
  assert(m_meshes->size() > 0);
  glm::vec3 min(INFINITY), max(-INFINITY);
  for (const auto& mesh : *m_meshes)
  {
    for (const auto& v : mesh.vertices())
    {
      min = glm::min(min, v.position);
      max = glm::max(max, v.position);
    }
  }
  m_center = (min + max) * 0.5f;
  // TODO: get rid of this stupidity. need more elegant way of handling geometry changes
  const_cast<Object3D&>(*this).set_flag(GEOMETRY_MODIFIED, false);
  return m_center;
}

void Object3D::apply_shading(ShadingProcessor::ShadingMode mode)
{
  if (get_flag(IS_FIXED_SHADING))
    return;
  if (mode != m_shading_mode)
  {
    // if current mesh is not cached
    if (!m_cached_meshes[m_shading_mode])
    {
      m_cached_meshes[m_shading_mode] = m_meshes;
    }
    // if mesh with requested shading mode is already in cache
    if (auto mesh_ptr_from_cache = m_cached_meshes[mode])
    {
      m_meshes = mesh_ptr_from_cache;
      m_shading_mode = mode;
      return;
    }
    // copy data
    auto current_meshes_tmp = m_meshes;
    m_meshes = std::make_shared<std::vector<Mesh>>();
    for (const auto& mesh : *current_meshes_tmp)
    {
      m_meshes->emplace_back(mesh.vertices(), mesh.faces());
    }
    ShadingProcessor::apply_shading(*m_meshes, mode);
    m_shading_mode = mode;
  }
}
