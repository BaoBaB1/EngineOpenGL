#pragma once

#include "./ge/Object3D.hpp"

struct Vertex;

class Polyline : public Object3D {
public:
  Polyline();
  bool has_surface() const override { return false; }
  std::optional<RayHit> hit(const Ray& ray) const override { return {}; }
  void add(const Vertex& point);
};
