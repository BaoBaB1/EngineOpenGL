#pragma once

#include "./ge/Object3D.hpp"

struct Vertex;

class Polyline : public Object3D {
public:
  Polyline();
  std::string name() const override { return "Polyline"; }
  bool has_surface() const override { return false; }
  void add(const Vertex& point);
};
