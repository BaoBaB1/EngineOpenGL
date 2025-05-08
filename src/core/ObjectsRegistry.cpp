#include "ObjectsRegistry.hpp"
#include "ge/BezierCurve.hpp"
#include "ge/Pyramid.hpp"
#include "ge/Icosahedron.hpp"
#include "ge/Polyline.hpp"
#include "ge/Cube.hpp"

namespace
{
  using namespace fury;
  uint32_t dummy0 = ObjectsRegistry::register_type<Object3D>();
  uint32_t dummy1 = ObjectsRegistry::register_type<Cube>();
  uint32_t dummy2 = ObjectsRegistry::register_type<Icosahedron>();
  uint32_t dummy3 = ObjectsRegistry::register_type<Pyramid>();
  uint32_t dummy4 = ObjectsRegistry::register_type<BezierCurve>();
  uint32_t dummy5 = ObjectsRegistry::register_type<Polyline>();
}
