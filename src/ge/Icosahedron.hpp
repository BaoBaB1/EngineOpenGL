#pragma once

#include "Object3D.hpp"

namespace fury
{
  class Icosahedron : public Object3D
  {
  public:
    inline constexpr static int32_t type = 2;
    Icosahedron();
    int32_t get_type() const override { return type; }
    void subdivide_triangles(int subdivision_depth);
    void project_points_on_sphere();
  private:
    void subdivide_triangles(int subdivision_level, const Vertex& a, const Vertex& b, const Vertex& c);
    void allocate_memory_before_subdivision(int subdivision_depth, int face_count);
  };
}
