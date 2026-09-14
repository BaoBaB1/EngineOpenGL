#pragma once

#include "core/Macros.hpp"
#include "core/Serialization.hpp"
#include <glm/glm.hpp>
#include <type_traits>

namespace fury
{
  namespace ecs
  {
    enum LightType : int32_t
    {
      DIRECTIONAL,
      POINT,
      SPOT,
      UNKNOWN,
      LAST_ITEM
    };

    struct LightComponent
    {
      FURY_REGISTER_COMPONENT(LightComponent)

      // match GLSL 430 layout padding
      glm::vec4 position;
      glm::vec4 dir;
      glm::vec4 ambient = glm::vec4(1.f);
      glm::vec4 diffuse = glm::vec4(1.f);
      glm::vec4 specular = glm::vec4(1.f);
      // matrix for transformation to light space
      glm::mat4 shadow_matrix;

      // attenuation info for point light
      float constant = 1.f;
      float linear = 0.14f;
      float quadratic = 0.07f;

      // cutoff for spot light (cos(angle))
      float cutoff = 0.9654321f;
      float outerCutoff = 0.9f;
      int32_t smoothSpotLight = true;

      LightType type = LightType::UNKNOWN;
      int32_t is_active = true;
    };
    // Make sure to match GLSL alignment
    static_assert(sizeof(LightComponent) % 16 == 0);

  } // namespace ecs
} // namespace fury
