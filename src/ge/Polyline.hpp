#pragma once

#include "./ge/Object3D.hpp"
#include "Vertex.hpp"

namespace fury
{
  class Polyline : public Object3D {
  public:
    FURY_REGISTER_DERIVED_CLASS(Polyline, Object3D)
    Polyline();
    Polyline(std::initializer_list<Vertex> points);
    Polyline(const std::vector<Vertex>& points);
    void add(const Vertex& point);
    const std::vector<Vertex>& get_points() const { return get_mesh(0).vertices(); }
  };
}
