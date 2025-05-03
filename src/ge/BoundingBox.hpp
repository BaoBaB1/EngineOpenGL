#pragma once

#include "Entity.hpp"
#include "IRayHittable.hpp"
#include "ge/Vertex.hpp"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <array>

namespace fury
{
  class BoundingBox : public IRayHittable
  {
  public:
    BoundingBox();
    BoundingBox(const glm::vec3& min, const glm::vec3& max);
    void init(const glm::vec3& min, const glm::vec3& max);
    void grow(const glm::vec3& min, const glm::vec3& max);
    void reset();
    std::optional<RayHit> hit(const Ray& ray) const override;
    std::array<glm::vec3, 8> get_points() const;
    bool is_empty() const;
    bool contains(const glm::vec3& point) const;
    glm::vec3& min() { return m_min; }
    glm::vec3& max() { return m_max; }
    glm::vec3 center() const { return (m_min + m_max) * 0.5f; }
    const glm::vec3& min() const { return m_min; }
    const glm::vec3& max() const { return m_max; }
    operator bool() const { return !is_empty(); }
  private:
    glm::vec3 m_min;
    glm::vec3 m_max;
  };
}
