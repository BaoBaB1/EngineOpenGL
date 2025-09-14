#pragma once

#include <glm/glm.hpp>

namespace fury
{
  struct ObjectChangeInfo
  {
    bool is_shading_mode_change = false;
    //bool is_geometry_change = false;
    bool is_vertex_change = false;
    bool is_transformation_change = false;
    glm::vec3 position_change;
    glm::vec3 scale_change;
    glm::vec3 rotation_axis_change;
    float rotation_angle_change = 0;
  };
}
