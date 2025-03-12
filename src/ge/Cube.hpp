#pragma once

#include "Object3D.hpp"

class Cube : public Object3D
{
public:
  Cube();
  bool has_surface() const override { return true; }
  std::string name() const override { return "Cube"; }
};
