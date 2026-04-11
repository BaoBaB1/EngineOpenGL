#pragma once

#include <glm/glm.hpp>

namespace fury
{
  struct Plane {
    glm::vec3 normal;
    float distance = 0.f;
  };
}
