#pragma once

#include "Object3D.hpp"

class ComplexModel : public Object3D
{
public:
  bool has_surface() const override { return true; }
  std::string name() const override { return "ComplexModel"; }
};
