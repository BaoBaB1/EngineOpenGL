#pragma once

#include "./ge/Object3D.hpp"

namespace fury
{
  struct Vertex;

  class Polyline : public Object3D {
  public:
    inline constexpr static int32_t type = 5;
    Polyline();
    bool has_surface() const override { return false; }
    int get_type() const override { return type; }
    std::optional<RayHit> hit(const Ray& ray) const override { return {}; }
    void add(const Vertex& point);
  };
}
