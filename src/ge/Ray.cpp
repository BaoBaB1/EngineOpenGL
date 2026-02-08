#include "Ray.hpp"
#include "ge/Object3D.hpp"
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
    // https://tavianator.com/2015/ray_box_nan.html
    glm::vec3 inv_dir = 1.f / m_dir;
    float t1 = (bbox.min().x - m_origin[0]) * inv_dir.x;
    float t2 = (bbox.max().x - m_origin[0]) * inv_dir.x;
    float tmin = std::min(t1, t2);
    float tmax = std::max(t1, t2);
    for (int i = 1; i < 3; ++i)
    {
      t1 = (bbox.min()[i] - m_origin[i]) * inv_dir[i];
      t2 = (bbox.max()[i] - m_origin[i]) * inv_dir[i];
      tmin = std::max(tmin, std::min(t1, t2));
      tmax = std::min(tmax, std::max(t1, t2));
    }

    if (tmax > std::max(tmin, 0.0f))
    {
      if (tmin < 0)
        tmin = tmax;
      RayHit hit;
      hit.position = m_origin + tmin * m_dir;
      hit.distance = glm::distance(m_origin, hit.position);
      hit.normal = glm::vec3(0.f);
      return hit;
    }
    return {};
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

  std::optional<RayHit> Ray::intersect_object3d(const Object3D* obj) const
  {
    const BoundingBox& aabb = obj->get_bbox();
    if (aabb.is_empty())
    {
      const_cast<Object3D*>(obj)->calculate_bbox();
    }
    std::optional<RayHit> rhit;
    if (intersect_aabb(aabb))
    {
      const auto& cfg = obj->get_render_config();
      if (cfg.mode == GL_TRIANGLES)
      {
        if (cfg.use_indices)
        {
          for (const auto& mesh : obj->get_meshes())
          {
            for (const auto& face : mesh.faces())
            {
              if (auto hit = intersect_triangle(
                mesh.get_vertex(face[0]).position, mesh.get_vertex(face[1]).position, mesh.get_vertex(face[2]).position)
                )
              {
                // find closest hit
                if (!rhit || rhit->distance > hit->distance)
                  rhit = hit;
              }
            }
          }
        }
        else
        {
          for (const auto& mesh : obj->get_meshes())
          {
            const auto& vertices = mesh.vertices();
            for (size_t i = 0; i < vertices.size(); i += 3)
            {
              if (auto hit = intersect_triangle(
                mesh.get_vertex(i).position, mesh.get_vertex(i + 1).position, mesh.get_vertex(i + 2).position)
                )
              {
                // find closest hit
                if (!rhit || rhit->distance > hit->distance)
                  rhit = hit;
              }
            }
          }
        }
      }
      else
      {
        Logger::info("Bounding box of object {} is intersected, but could not test if object is actually hit. Primitives are not triangles", obj->get_name());
      }
    }
    return rhit;
  }
}
