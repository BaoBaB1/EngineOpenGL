#include "Ray.hpp"
#include "ge/BoundingBox.hpp"
#include <glm/gtx/intersect.hpp>

namespace fury
{
  std::optional<RayHit> Ray::intersect_triangle(const glm::vec3& v, const glm::vec3& v2, const glm::vec3& v3) const
  {
    glm::vec2 barycentric;
    float distance;
    if (!glm::intersectRayTriangle(m_origin, m_dir, v, v2, v3, barycentric, distance))
    {
      return {};
    }
    RayHit hit;
    hit.distance = distance;
    hit.position = (1.f - barycentric.x - barycentric.y) * v + barycentric.x * v2 + barycentric.y * v3;
    hit.normal = glm::normalize(glm::cross(v2 - v, v3 - v));
    return hit;
  }

  std::optional<RayHit> Ray::intersect_plane(const glm::vec3& plane_origin, const glm::vec3& plane_normal) const
  {
    float distance;
    if (!glm::intersectRayPlane(m_origin, m_dir, plane_origin, plane_normal, distance))
    {
      return {};
    }
    RayHit hit;
    hit.distance = distance;
    hit.position = m_origin + m_dir * distance;
    hit.normal = plane_normal;
    return hit;
  }

  std::optional<RayHit> Ray::intersect_aabb(const BoundingBox& bbox) const
  {
    // (https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection.html)
    // TODO: some better impl without that many ifs
    glm::vec3 tmin = (bbox.min() - m_origin) / m_dir;
    glm::vec3 tmax = (bbox.max() - m_origin) / m_dir;
    if (tmin.x > tmax.x)
    {
      std::swap(tmin.x, tmax.x);
    }
    if (tmin.y > tmax.y)
    {
      std::swap(tmin.y, tmax.y);
    }
    if (tmin.z > tmax.z)
    {
      std::swap(tmin.z, tmax.z);
    }

    if ((tmin.x > tmax.y) || (tmin.y > tmax.x))
    {
      return {};
    }

    if (tmin.y > tmin.x)
    {
      tmin.x = tmin.y;
    }
    if (tmax.y < tmax.x)
    {
      tmax.x = tmax.y;
    }

    if ((tmin.x > tmax.z) || (tmin.z > tmax.x))
    {
      return {};
    }

    if (tmin.z > tmin.x)
    {
      tmin.x = tmin.z;
    }
    if (tmax.z < tmax.x)
    {
      tmax.x = tmax.z;
    }

    if (tmin.x < 0)
    {
      if (tmax.x < 0)
      {
        return {};
      }
      // 1 point of intersection
      tmin.x = tmax.x;
    }

    if (tmin.x > tmax.x)
    {
      std::swap(tmin.x, tmax.x);
    }

    RayHit hit;
    hit.position = m_origin + tmin.x * m_dir;
    hit.distance = glm::distance(m_origin, hit.position);
    hit.normal = glm::vec3(0.f);
    return hit;
  }

  std::optional<RayHit> Ray::intersect_sphere(const glm::vec3& center, float radius) const
  {
    RayHit hit;
    if (!glm::intersectRaySphere(m_origin, m_dir, center, radius, hit.position, hit.normal))
    {
      return {};
    }
    hit.distance = glm::distance(m_origin, hit.position);
    return hit;
  }
}
