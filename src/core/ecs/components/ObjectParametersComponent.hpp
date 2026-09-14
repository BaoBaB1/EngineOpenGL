#pragma once

#include "core/Macros.hpp"
#include <glm/glm.hpp>

namespace fury
{
  enum class BezierCurveType
  {
    Quadratic,
    Cubic
  };

  namespace ecs
  {
    struct CubeParametersComponent
    {
      FURY_REGISTER_COMPONENT(CubeParametersComponent)
    };

    struct PyramidParametersComponent
    {
      FURY_REGISTER_COMPONENT(PyramidParametersComponent)
    };

    struct IcosahedronParametersComponent
    {
      FURY_REGISTER_COMPONENT(IcosahedronParametersComponent)
      uint32_t subdivisiton_depth = 0;
      bool project_points_on_sphere = false;
    };

    struct BezierCurveParametersComponent
    {
      FURY_REGISTER_COMPONENT(BezierCurveParametersComponent)
      glm::vec3 start_point;
      glm::vec3 end_point;
      glm::vec3 control_point1;
      glm::vec3 control_point2;
      BezierCurveType curve_type = BezierCurveType::Quadratic;
    };
  } // namespace ecs
} // namespace fury
