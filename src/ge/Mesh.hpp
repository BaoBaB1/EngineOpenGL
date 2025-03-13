#pragma once

#include <glad/glad.h>
#include "ge/Vertex.hpp"
#include "ge/Face.hpp"
#include "ge/BoundingBox.hpp"
#include "core/Texture2D.hpp"
#include "core/Material.hpp"
#include <vector>
#include <optional>
#include <memory>
#include <array>


class Mesh {
public:
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
  std::optional<Material> material() { return m_material; }
  const std::optional<Material> material() const { return m_material; }
  size_t append_vertex(const Vertex& vertex);
  size_t append_face(const Face& face);
  size_t append_face(Face&& face);
private:
  std::optional<Material> m_material;
  std::vector<Vertex> m_vertices;
  std::vector<Face> m_faces;
  mutable std::vector<GLuint> m_faces_indices;
  //std::vector<Vertex> m_cached_normals;     // normal lines
  std::array<std::shared_ptr<Texture2D>, static_cast<int>(TextureType::LAST) + 1> m_textures;
  BoundingBox m_bbox;
};
