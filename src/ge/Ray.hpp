#pragma once

#include <glm/glm.hpp>
#include <optional>

namespace fury
{
  class Object3D;
  class BoundingBox;

  struct RayHit
  {
    float distance = 0.f;
    glm::vec3 position = glm::vec3(0.);
    glm::vec3 normal = glm::vec3(0.);
  };

  struct Ray
  {
    Ray() = default;
    Ray(const glm::vec3& origin, const glm::vec3& dir) : m_origin(origin), m_dir(glm::normalize(dir)) {}
    std::optional<RayHit> intersect_triangle(const glm::vec3& v, const glm::vec3& v2, const glm::vec3& v3) const;
    std::optional<RayHit> intersect_plane(const glm::vec3& plane_origin, const glm::vec3& plane_normal) const;
    std::optional<RayHit> intersect_aabb(const BoundingBox& bbox) const;
    std::optional<RayHit> intersect_sphere(const glm::vec3& center, float radius) const;
    glm::vec3& get_origin() { return m_origin; }
    glm::vec3& get_direction() { return m_dir; }
  private:
    glm::vec3 m_origin;
    glm::vec3 m_dir;
  };
}
