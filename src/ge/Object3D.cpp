#include "Object3D.hpp"

Object3D::Object3D()
{
  //m_meshes.resize(1);
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

void Object3D::add_mesh(Mesh&& mesh)
{ 
  if (mesh.material())
    set_flag(HAS_MATERIAL, true);
  set_flag(GEOMETRY_MODIFIED, true);
  m_meshes->push_back(std::move(mesh)); 
}

void Object3D::add_mesh(const Mesh& mesh)
{
  if (mesh.material())
    set_flag(HAS_MATERIAL, true);
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
  m_bbox.set_min(min);
  m_bbox.set_max(max);
}

void Object3D::set_texture(const std::shared_ptr<Texture2D>& tex, TextureType type, size_t mesh_idx)
{
  (*m_meshes)[mesh_idx].set_texture(tex, type);
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

void Object3D::apply_shading(Object3D::ShadingMode mode)
{
  if (mode != m_shading_mode)
  {
    if (!m_cached_meshes[m_shading_mode])
    {
      m_cached_meshes[m_shading_mode] = m_meshes;
    }

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

    // duplicate each vertex of current mesh for flat shading so each face has its own normal
    if (mode == Object3D::ShadingMode::FLAT_SHADING)
    {
      for (auto& mesh : *m_meshes)
      {
        m_vertex_finder.m_map_vert.clear();
        std::vector<GLuint> indices(3);
        std::vector<Face>& faces = mesh.faces(), new_faces;
        assert(faces.size() > 0);
        for (const auto& face : faces)
        {
          assert(face.size == 3);
          for (int i = 0; i < face.size; ++i)
          {
            Vertex vert = mesh.vertices()[face.data[i]];
            VertexFinder::iter iter = m_vertex_finder.find_vertex(vert);
            // this vertex already present
            if (iter != m_vertex_finder.end())
            {
              // add copy
              indices[i] = static_cast<GLuint>(mesh.append_vertex(vert));
            }
            else
            {
              indices[i] = face.data[i];
              m_vertex_finder.add_vertex(vert, indices[i]);
            }
          }
          new_faces.push_back(indices);
        }
        faces = std::move(new_faces);
        calc_normals(mesh, mode);
      }
    }
    // for smooth shading we have to make sure that every vertex is unique as well as it's normal.
    // fragment color will be interpolated between triangle's vertex normals
    // same applies for shading mode = NO_SHADING, except the difference that normals are 0,0,0
    else
    {
      for (auto& mesh : *m_meshes)
      {
        m_vertex_finder.m_map_vert.clear();
        std::vector<GLuint> indices(3);
        std::vector<Vertex>& vertices = mesh.vertices(), unique_vertices;
        std::vector<Face>& faces = mesh.faces();
        assert(faces.size() > 0);
        for (auto& face : faces)
        {
          indices.resize(face.size);
          for (int i = 0; i < face.size; ++i)
          {
            Vertex vert = vertices[face.data[i]];
            vert.normal = glm::vec3(0.f);
            VertexFinder::iter iter = m_vertex_finder.find_vertex(vert);
            if (iter != m_vertex_finder.end())
            {
              face.data[i] = iter->second;
            }
            else
            {
              unique_vertices.push_back(vert);
              face.data[i] = unique_vertices.size() - 1;
              m_vertex_finder.add_vertex(vert, face.data[i]);
            }
          }
        }
        vertices = std::move(unique_vertices);
        if (mode == Object3D::ShadingMode::SMOOTH_SHADING)
        {
          calc_normals(mesh, mode);
        }
      }
    }
    m_cached_meshes[mode] = m_meshes;
    m_shading_mode = mode;
  }
}

void Object3D::calc_normals(Mesh& mesh, ShadingMode mode)
{
  if (mode == ShadingMode::NO_SHADING)
    return;
  std::vector<Vertex>& vertices = mesh.vertices();
  const std::vector<Face>& faces = mesh.faces();
  for (auto& vert : vertices)
  {
    vert.normal = glm::vec3(0.f);
  }
  for (auto& face : faces)
  {
    assert(face.size == 3);
    GLuint ind = face.data[0], ind2 = face.data[1], ind3 = face.data[2];
    glm::vec3 a = vertices[ind2].position - vertices[ind].position;
    glm::vec3 b = vertices[ind3].position - vertices[ind].position;
    glm::vec3 normal = glm::cross(a, b);
    // TODO: some triangles may be in CW order while other in CCW and it affects on normal.
    // so here would be nice somehow check if normal is pointing inside or outside.
    // providing such functionality will avoid defining all faces in CW or CCW order
    // as their normals will always point outside despite their order given in constructor
    // AND/OR make all faces in same winding if there are some in different
    if (mode == ShadingMode::SMOOTH_SHADING)
    {
      vertices[ind].normal += normal;
      vertices[ind2].normal += normal;
      vertices[ind3].normal += normal;
    }
    else if (ShadingMode::FLAT_SHADING)
    {
      vertices[ind].normal = normal;
      vertices[ind2].normal = normal;
      vertices[ind3].normal = normal;
    }
  }
  for (Vertex& v : vertices)
    if (v.normal != glm::vec3())
      v.normal = glm::normalize(v.normal);
}
