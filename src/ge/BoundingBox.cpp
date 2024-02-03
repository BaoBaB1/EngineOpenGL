#include "utils/Constants.hpp"
#include "BoundingBox.hpp"

static constexpr float g_fmin = OpenGLEngineUtils::limits::fmin;
static constexpr float g_fmax = OpenGLEngineUtils::limits::fmax;

BoundingBox::BoundingBox()
{
  m_min = glm::vec3(g_fmin, g_fmin, g_fmin);
  m_max = glm::vec3(g_fmax, g_fmax, g_fmax);
}

bool BoundingBox::is_empty() const 
{
  return m_min == glm::vec3(g_fmin, g_fmin, g_fmin) && m_max == glm::vec3(g_fmax, g_fmax, g_fmax);
}

std::array<glm::vec3, 8> BoundingBox::points() const
{
  std::array<glm::vec3, 8> points; 
  // llc - left lower corner, rtc - right top corner ...
  // front quad (llc -> rlc -> rtc -> ltc) 
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

std::vector<GLuint> BoundingBox::lines_indices() const
{
  std::vector<GLuint> v = {
    0, 1, 1, 2, 2, 3, 3, 0, // front
    4, 5, 5, 6, 6, 7, 7, 4, // back
    0, 4, 3, 7, 1, 5, 2, 6
  };
  return v;
}
