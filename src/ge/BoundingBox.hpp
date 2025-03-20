#pragma once

#include <array>
#include <glm/glm.hpp>
#include "Entity.hpp"
#include "IRayHittable.hpp"
#include "./ge/Face.hpp"

class BoundingBox : public Entity, public IRayHittable
{
public:
  BoundingBox();
  BoundingBox(const glm::vec3& min, const glm::vec3 max) : Entity("Bounding box") { m_min = min; m_max = max; }
  std::optional<RayHit> hit(const Ray& ray) const override;
  std::array<glm::vec3, 8> points() const;
  std::vector<GLuint> lines_indices() const;
  bool is_empty() const;
  bool contains(const glm::vec3& point) const;
  void set_min(const glm::vec3& min) { m_min = min; }
  void set_max(const glm::vec3& max) { m_max = max; }
  glm::vec3 min() const { return m_min; }
  glm::vec3 max() const { return m_max; }
  operator bool() const { return !is_empty(); }
private:
  glm::vec3 m_min;
  glm::vec3 m_max;
};