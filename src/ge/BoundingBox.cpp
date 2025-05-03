#include "utils/Constants.hpp"
#include "BoundingBox.hpp"

namespace fury
{
  BoundingBox::BoundingBox()
  {
    init(glm::vec3(fury::constants::fmax), glm::vec3(fury::constants::fmin));
  }

  BoundingBox::BoundingBox(const glm::vec3& min, const glm::vec3& max)
  {
    init(min, max);
  }

  bool BoundingBox::is_empty() const
  {
    return m_min == glm::vec3(fury::constants::fmax) && m_max == glm::vec3(fury::constants::fmin);
  }

  std::optional<RayHit> BoundingBox::hit(const Ray& ray) const
  {
    return ray.intersect_aabb(*this);
  }

  std::array<glm::vec3, 8> BoundingBox::get_points() const
  {
    // llc - left lower corner, rtc - right top corner ...
    // front quad (llc -> rlc -> rtc -> ltc) 
    std::array<glm::vec3, 8> points;
    points[0] = m_min;
    points[1] = glm::vec3(m_max.x, m_min.y, m_min.z);
    points[2] = glm::vec3(m_max.x, m_max.y, m_min.z);
    points[3] = glm::vec3(m_min.x, m_max.y, m_min.z);
    // back quad (llc -> rlc -> rtc -> ltc)
    points[4] = glm::vec3(m_min.x, m_min.y, m_max.z);
    points[5] = glm::vec3(m_max.x, m_min.y, m_max.z);
    points[6] = m_max;
    points[7] = glm::vec3(m_min.x, m_max.y, m_max.z);
    return points;
  }

  bool BoundingBox::contains(const glm::vec3& point) const
  {
    return (point.x >= m_min.x && point.x <= m_max.x) &&
      (point.y >= m_min.y && point.y <= m_max.y) &&
      (point.z >= m_min.z && point.z <= m_max.z);
  }

  void BoundingBox::init(const glm::vec3& min, const glm::vec3& max)
  {
    m_min = min;
    m_max = max;
  }

  void BoundingBox::grow(const glm::vec3& min, const glm::vec3& max)
  {
    m_min = glm::min(min, m_min);
    m_max = glm::max(max, m_max);
  }

  void BoundingBox::reset()
  {
    init(glm::vec3(fury::constants::fmax), glm::vec3(fury::constants::fmin));
  }
}
