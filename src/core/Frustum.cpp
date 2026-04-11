#include "Frustum.hpp"
#include "ge/BoundingBox.hpp"

namespace fury
{
  bool Frustum::is_inside(const BoundingBox& aabb) const
  {
    const auto points = aabb.get_points();
    auto is_fully_outside = [&](const Plane& plane)
      {
        int out = 0;
        for (const glm::vec3& point : points) {
          out += glm::dot(glm::vec4(plane.normal, plane.distance), glm::vec4(point, 1.f)) < 0;
        }
        return out == 8;
      };
    const std::array planes = { &near, &far, &left, &right, &top, &bottom };
    for (const Plane* plane : planes)
    {
      if (is_fully_outside(*plane)) {
        return false;
      }
    }
    return true;
  }
}
