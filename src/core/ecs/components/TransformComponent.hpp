#pragma once

#include "core/Macros.hpp"
#include "core/Serialization.hpp"
#include "core/SceneGraph.hpp"

namespace fury
{
  namespace ecs
  {
    struct TransformComponent
    {
      FURY_REGISTER_COMPONENT(TransformComponent)
      FURY_DECLARE_SERIALIZABLE_FIELDS(FURY_SERIALIZABLE_FIELD(1, &TransformComponent::transform_node))
      TransformationSceneNode transform_node;
    };
  } // namespace ecs
} // namespace fury
