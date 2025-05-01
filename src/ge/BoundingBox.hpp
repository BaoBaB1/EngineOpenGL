#pragma once

#include "Entity.hpp"
#include "IRayHittable.hpp"
#include "ge/Vertex.hpp"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <array>

namespace fury
{
  class BoundingBox : public Entity, public IRayHittable
  {
  public:
    static const std::array<GLuint, 24>& lines_indices();
    BoundingBox();
    BoundingBox(const glm::vec3& min, const glm::vec3& max);
    void init(const glm::vec3& min, const glm::vec3& max);
    void grow(const glm::vec3& min, const glm::vec3& max);
    void reset();
    std::optional<RayHit> hit(const Ray& ray) const override;
    const std::array<Vertex, 8>& points() const { return m_points; }
    bool is_empty() const;
    bool contains(const glm::vec3& point) const;
    glm::vec3& min() { return m_min; }
    glm::vec3& max() { return m_max; }
    glm::vec3 center() const { return (m_min + m_max) / 2.f; }
    const glm::vec3& min() const { return m_min; }
    const glm::vec3& max() const { return m_max; }
    operator bool() const { return !is_empty(); }
  private:
    void set_points();
  private:
    // for convenient vbo setup
    // TODO: move to other place ?
    std::array<Vertex, 8> m_points;
    glm::vec3 m_min;
    glm::vec3 m_max;
  };
}
