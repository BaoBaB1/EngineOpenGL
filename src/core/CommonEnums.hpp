#pragma once

namespace fury
{
  enum class ShadingMode
  {
    NO_SHADING,
    FLAT_SHADING,
    SMOOTH_SHADING,
    PREDEFINED, // e.g. in model loaded from file
    LAST_ITEM
  };
} // namespace fury
