#pragma once

#include "Entity.hpp"
#include "IRayHittable.hpp"
#include "ge/Vertex.hpp"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <array>

namespace fury
{
  class BoundingBox : public Entity, public IRayHittable
  {
  public:
    BoundingBox();
    BoundingBox(const glm::vec3& min, const glm::vec3& max);
    void init(const glm::vec3& min, const glm::vec3& max);
    static const std::array<GLuint, 24>& lines_indices();
    std::optional<RayHit> hit(const Ray& ray) const override;
    const std::vector<Vertex>& points() const { return m_points; }
    bool is_empty() const;
    bool contains(const glm::vec3& point) const;
    glm::vec3 min() const { return m_min; }
    glm::vec3 max() const { return m_max; }
    operator bool() const { return !is_empty(); }
  private:
    // for convenient vbo setup
    std::vector<Vertex> m_points;
    glm::vec3 m_min;
    glm::vec3 m_max;
  };
}
