#pragma once

#include "./ge/Object3D.hpp"
#include "Vertex.hpp"

namespace fury
{
  struct Vertex;

  class Polyline : public Object3D {
  public:
    Polyline();
    Polyline(std::initializer_list<Vertex> points);
    uint32_t get_type() const override { return ObjectsRegistry::get_id<Polyline>(); }
    std::optional<RayHit> hit(const Ray& ray) const override { return {}; }
    void add(const Vertex& point);
    const std::vector<Vertex>& get_points() const { return get_mesh(0).vertices(); }
  };
}
