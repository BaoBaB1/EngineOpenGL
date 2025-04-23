#include "utils/Constants.hpp"
#include "BoundingBox.hpp"

static constexpr float g_fmin = furyutils::limits::fmin;
static constexpr float g_fmax = furyutils::limits::fmax;

namespace fury
{
  BoundingBox::BoundingBox() : Entity("Bounding box")
  {
    init(glm::vec3(g_fmax), glm::vec3(g_fmin));
  }

  BoundingBox::BoundingBox(const glm::vec3& min, const glm::vec3& max) : Entity("Bounding box")
  {
    init(min, max);
  }

  bool BoundingBox::is_empty() const
  {
    return m_min == glm::vec3(g_fmax) && m_max == glm::vec3(g_fmin);
  }

  std::optional<RayHit> BoundingBox::hit(const Ray& ray) const
  {
    return ray.intersect_aabb(*this);
  }

  bool BoundingBox::contains(const glm::vec3& point) const
  {
    return (point.x >= m_min.x && point.x <= m_max.x) &&
      (point.y >= m_min.y && point.y <= m_max.y) &&
      (point.z >= m_min.z && point.z <= m_max.z);
  }

  void BoundingBox::set_points()
  {
    // llc - left lower corner, rtc - right top corner ...
    // front quad (llc -> rlc -> rtc -> ltc) 
    m_points[0] = m_min;
    m_points[1] = glm::vec3(m_max.x, m_min.y, m_min.z);
    m_points[2] = glm::vec3(m_max.x, m_max.y, m_min.z);
    m_points[3] = glm::vec3(m_min.x, m_max.y, m_min.z);
    // back quad (llc -> rlc -> rtc -> ltc)
    m_points[4] = glm::vec3(m_min.x, m_min.y, m_max.z);
    m_points[5] = glm::vec3(m_max.x, m_min.y, m_max.z);
    m_points[6] = m_max;
    m_points[7] = glm::vec3(m_min.x, m_max.y, m_max.z);
  }

  void BoundingBox::init(const glm::vec3& min, const glm::vec3& max)
  {
    m_min = min;
    m_max = max;
    set_points();
  }

  void BoundingBox::grow(const glm::vec3& min, const glm::vec3& max)
  {
    m_min = glm::min(min, m_min);
    m_max = glm::max(max, m_max);
    set_points();
  }

  void BoundingBox::reset()
  {
    init(glm::vec3(g_fmax), glm::vec3(g_fmin));
  }

  const std::array<GLuint, 24>& BoundingBox::lines_indices()
  {
    constexpr static std::array<GLuint, 24> indices = {
      0, 1, 1, 2, 2, 3, 3, 0, // front
      4, 5, 5, 6, 6, 7, 7, 4, // back
      0, 4, 3, 7, 1, 5, 2, 6
    };
    return indices;
  }
}
