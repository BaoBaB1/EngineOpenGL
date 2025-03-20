#pragma once

#include "Object3D.hpp"

class Pyramid : public Object3D {
public:
  Pyramid();
  bool has_surface() const override { return true; }
};
