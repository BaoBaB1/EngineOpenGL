#pragma once

#include "ge/Object3D.hpp"
#include <memory>

namespace fury
{
  class GeometryFactory
  {
  public:
    static std::unique_ptr<Object3D> create_from_type(int32_t type);
  };
}
