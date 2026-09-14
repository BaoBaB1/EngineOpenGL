#pragma once

#include "core/Macros.hpp"
#include <glm/glm.hpp>

namespace fury
{
  namespace ecs
  {
    struct RotationComponent
    {
      FURY_REGISTER_COMPONENT(RotationComponent)
      glm::vec3 axis = { 1, 0, 0 };
      float angle = 0.f;
    };
  } // namespace ecs
} // namespace fury
