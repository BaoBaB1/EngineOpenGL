#include "Mesh.hpp"

namespace fury
{
  Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<Face>& faces)
  {
    m_vertices = vertices;
    m_faces = faces;
  }

  size_t Mesh::append_vertex(const Vertex& vertex) {
    m_vertices.push_back(vertex);
    return m_vertices.size() - 1;
  }

  size_t Mesh::append_face(const Face& face)
  {
    m_faces.push_back(face);
    return m_faces.size() - 1;
  }

  size_t Mesh::append_face(Face&& face)
  {
    m_faces.push_back(std::move(face));
    return m_faces.size() - 1;
  }

  // this is called during rendering
  const std::vector<GLuint>& Mesh::faces_as_indices() const
  {
    if (m_faces_indices.empty())
    {
      m_faces_indices.reserve(m_faces.size() * 3);
      for (const auto& face : m_faces)
      {
        assert(face.size == 3);
        for (uint32_t i = 0; i < face.size; i++)
        {
          m_faces_indices.push_back(face.data[i]);
        }
      }
    }
    return m_faces_indices;
  }

  bool Mesh::has_texture() const
  {
    for (const auto& tex : m_textures)
    {
      if (tex)
        return true;
    }
    return false;
  }
}
