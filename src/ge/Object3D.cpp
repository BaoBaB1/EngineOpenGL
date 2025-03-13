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
  m_meshes.push_back(std::move(mesh)); 
}

void Object3D::add_mesh(const Mesh& mesh)
{
  if (mesh.material())
    set_flag(HAS_MATERIAL, true);
  set_flag(GEOMETRY_MODIFIED, true);
  m_meshes.push_back(mesh); 
}

std::vector<Vertex> Object3D::normals_as_lines()
{
  // Too slow to call every frame if object has a lot of vertices
  size_t vertices_count = 0;
  for (const auto& mesh : m_meshes)
  {
    vertices_count += mesh.vertices().size();
  }
  std::vector<Vertex> normals(vertices_count * 2);
  constexpr float len_scaler = 3.f;
  size_t index = 0;
  for (const auto& mesh : m_meshes)
  {
    for (const auto& vertex : mesh.vertices())
    {
      normals[index].position = vertex.position;
      normals[index + 1].position = vertex.position + vertex.normal / len_scaler;
      normals[index].color = normals[index + 1].color = glm::vec4(0.f, 1.f, 1.f, 1.f);
      index += 2;
    }
  }
  return normals;
}

void Object3D::calculate_bbox(bool force)
{
  if (!force && !m_bbox.is_empty())
  {
    return;
  }
  glm::vec3 min(INFINITY), max(-INFINITY);
  for (const auto& mesh : m_meshes)
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
  m_meshes[mesh_idx].set_texture(tex, type);
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
  apply_color(m_meshes, color);

  // set color of cached meshes
  for (auto& cached_meshes : m_cached_meshes)
  {
    apply_color(cached_meshes.second, color);
  }
}

glm::vec3 Object3D::center() const
{
  if (!get_flag(GEOMETRY_MODIFIED))
  {
    return m_center;
  }
  assert(m_meshes.size() > 0);
  glm::vec3 min(INFINITY), max(-INFINITY);
  for (const auto& mesh : m_meshes)
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
    // if meshes with current shading mode are not cached yet
    if (m_cached_meshes.count(m_shading_mode) == 0)
    {
      m_cached_meshes[m_shading_mode] = m_meshes;
    }
    m_shading_mode = mode;

    // if meshes with new shading mode have already been cached
    if (m_cached_meshes.count(mode) != 0)
    {
      m_meshes = m_cached_meshes[mode];
      return;
    }

    // no shading, all normals == 0
    if (mode == Object3D::ShadingMode::NO_SHADING)
    {
      for (auto& mesh : m_meshes)
      {
        for (Vertex& v : mesh.vertices())
        {
          v.normal = glm::vec3(0.f);
        }
      }
    }

    // duplicate each vertex of current mesh for flat shading so each face has its own normal
    else if (mode == Object3D::ShadingMode::FLAT_SHADING)
    {
      for (auto& mesh : m_meshes)
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
    else if (mode == Object3D::ShadingMode::SMOOTH_SHADING)
    {
      for (auto& mesh : m_meshes)
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
        calc_normals(mesh, mode);
      }
    }
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
