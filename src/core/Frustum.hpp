#pragma once

#include "Plane.hpp"
#include <utility>
#include <vector>

namespace fury
{
  class BoundingBox;

  struct Frustum {
    Plane near;
    Plane far;
    Plane left;
    Plane right;
    Plane top;
    Plane bottom;
    std::vector<std::pair<glm::vec3, glm::vec3>> debug_lines;
    bool is_inside(const BoundingBox& aabb) const;
  };
}
