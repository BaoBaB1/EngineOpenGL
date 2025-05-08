#pragma once

#include <vector>
#include "ge/Object3D.hpp"
#include "ge/Vertex.hpp"

namespace fury
{
  class Curve : public Object3D {
  public:
    Curve();
    Curve(const Vertex& start, const Vertex& end);
    std::optional<RayHit> hit(const Ray& ray) const override { return {}; }
    void set_start_point(const Vertex& start_pnt) { m_start_pnt = start_pnt; }
    void set_end_point(const Vertex& end_pnt) { m_end_pnt = end_pnt; }
    const Vertex& start_point() { return m_start_pnt; }
    const Vertex& end_point() { return m_end_pnt; }
  protected:
    Vertex m_start_pnt;
    Vertex m_end_pnt;
  };
}
