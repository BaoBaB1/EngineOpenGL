#pragma once

#include "Common.hpp"
#include "ecs/components/ObjectParametersComponent.hpp"
#include "ecs/components/DrawableComponent.hpp"
#include "Mesh.hpp"

namespace fury
{
  class GeometryFactory
  {
  public:
    static Mesh create_cube(const ecs::CubeParametersComponent& params, ShadingMode shading_mode);
    static Mesh create_pyramid(const ecs::PyramidParametersComponent& params, ShadingMode shading_mode);
    static Mesh create_icosahedron(const ecs::IcosahedronParametersComponent& params,
                                                ShadingMode shading_mode);
    static Mesh create_bezier_curve(const ecs::BezierCurveParametersComponent& params);
  };
} // namespace fury
