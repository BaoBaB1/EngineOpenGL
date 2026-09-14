#pragma once

#include "Common.hpp"
#include "Vertex.hpp"
#include "BoundingBox.hpp"
#include "Macros.hpp"
#include <vector>

namespace fury
{
  class Mesh
  {
  public:
    Mesh() = default;
    Mesh(const std::vector<Vertex>& vertices, const std::vector<Face>& faces);
    std::vector<Vertex>& get_vertices() { return m_vertices; }
    const std::vector<Vertex>& get_vertices() const { return m_vertices; }
    std::vector<Face>& get_faces() { return m_faces; }
    const std::vector<Face>& get_faces() const { return m_faces; }
    const std::vector<FaceIdxT>& faces_as_indices() const;
    void calculate_bbox();
    const BoundingBox& get_bbox() const { return m_bbox; }
    Vertex& get_vertex(size_t idx) { return m_vertices[idx]; }
    const Vertex& get_vertex(size_t idx) const { return m_vertices[idx]; }
    size_t append_vertex(const Vertex& vertex);
    size_t append_face(const Face& face);
    size_t append_face(Face&& face);
    FURY_DECLARE_SERIALIZABLE_FIELDS(FURY_SERIALIZABLE_FIELD(1, &Mesh::m_vertices),
                                     FURY_SERIALIZABLE_FIELD(2, &Mesh::m_faces),
                                     FURY_SERIALIZABLE_FIELD(3, &Mesh::m_bbox))
  private:
    std::vector<Vertex> m_vertices;
    std::vector<Face> m_faces;
    mutable std::vector<FaceIdxT> m_faces_indices;
    BoundingBox m_bbox;
  };
} // namespace fury
