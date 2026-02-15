#pragma once

#include "Object3D.hpp"

namespace fury
{
  class Icosahedron : public Object3D
  {
  public:
    FURY_REGISTER_DERIVED_CLASS(Icosahedron, Object3D)
    Icosahedron();
    void subdivide_triangles(int subdivision_depth);
    void project_points_on_sphere();
  private:
    void subdivide_triangles(int subdivision_level, const Vertex& a, const Vertex& b, const Vertex& c);
    void allocate_memory_before_subdivision(int subdivision_depth, int face_count);
  };
}
