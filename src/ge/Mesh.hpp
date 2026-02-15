#pragma once

#include <glad/glad.h>
#include "ge/Vertex.hpp"
#include "ge/Face.hpp"
#include "ge/BoundingBox.hpp"
#include "core/opengl/Texture2D.hpp"
#include "core/Material.hpp"
#include "core/Serialization.hpp"
#include <vector>
#include <memory>
#include <array>


namespace fury
{
  class Mesh {
  public:
    FURY_REGISTER_CLASS(Mesh)
    Mesh() = default;
    Mesh(const std::vector<Vertex>& vertices, const std::vector<Face>& faces);
    void set_material(const Material& material) { m_material = material; }
    std::vector<Vertex>& vertices() { return m_vertices; }
    const std::vector<Vertex>& vertices() const { return m_vertices; }
    std::vector<Face>& faces() { return m_faces; }
    const std::vector<Face>& faces() const { return m_faces; }
    const std::vector<GLuint>& Mesh::faces_as_indices() const;
    void set_texture(const std::shared_ptr<Texture2D>& tex, TextureType type) { m_textures[static_cast<int>(type)] = tex; }
    const std::shared_ptr<Texture2D> get_texture(TextureType type) const { return m_textures[static_cast<int>(type)]; }
    BoundingBox& bbox() { return m_bbox; }
    const BoundingBox& bbox() const { return m_bbox; }
    Material& material() { return m_material; }
    const Material& material() const { return m_material; }
    Vertex& get_vertex(size_t idx) { return m_vertices[idx]; }
    const Vertex& get_vertex(size_t idx) const { return m_vertices[idx]; }
    size_t append_vertex(const Vertex& vertex);
    size_t append_face(const Face& face);
    size_t append_face(Face&& face);
    FURY_DECLARE_SERIALIZABLE_FIELDS(
      FURY_SERIALIZABLE_FIELD(1, &Mesh::m_vertices),
      FURY_SERIALIZABLE_FIELD(2, &Mesh::m_faces),
      FURY_SERIALIZABLE_FIELD(3, &Mesh::m_material),
      FURY_SERIALIZABLE_FIELD(4, &Mesh::m_bbox),
      FURY_SERIALIZABLE_FIELD(5, &Mesh::m_textures)
    )
  private:
    Material m_material;
    std::vector<Vertex> m_vertices;
    std::vector<Face> m_faces;
    mutable std::vector<GLuint> m_faces_indices;
    std::array<std::shared_ptr<Texture2D>, static_cast<int>(TextureType::LAST) + 1> m_textures;
    BoundingBox m_bbox;
  };
}
