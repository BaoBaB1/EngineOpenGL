#pragma once

#include "CommonEnums.hpp"
#include "Mesh.hpp"

namespace fury
{
  class ShadingProcessor
  {
  public:
    static void apply_shading(Mesh& mesh, ShadingMode mode);
    static void calc_normals(Mesh& mesh, ShadingMode mode);
  };
} // namespace fury
