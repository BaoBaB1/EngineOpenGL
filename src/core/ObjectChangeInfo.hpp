#pragma once

namespace fury
{
  struct ObjectChangeInfo
  {
    bool is_shading_mode_change = false;
    //bool is_geometry_change = false;
    bool is_vertex_change = false;
    bool is_transformation_change = false;
  };
}
