#pragma once

#include "core/Macros.hpp"

namespace fury
{
  namespace ecs
  {
    struct DirtyEntityTagComponent
    {
      FURY_REGISTER_COMPONENT(DirtyEntityTagComponent)
      struct EntityDirtyState
      {
        uint32_t name_changed : 1;
        uint32_t color_changed : 1;
        uint32_t shading_mode_changed : 1;
        uint32_t transform_changed : 1;
      } dirty_bits;
    };
  } // namespace ecs
} // namespace fury
