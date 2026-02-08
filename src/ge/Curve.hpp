#pragma once

#include "ge/Object3D.hpp"

namespace fury
{
  class Curve : public Object3D {
  public:
    FURY_REGISTER_DERIVED_CLASS(Curve, Object3D)
    Curve();
    Curve(const Vertex& start, const Vertex& end);
    void set_start_point(const Vertex& start_pnt) { m_start_pnt = start_pnt; }
    void set_end_point(const Vertex& end_pnt) { m_end_pnt = end_pnt; }
    const Vertex& start_point() { return m_start_pnt; }
    const Vertex& end_point() { return m_end_pnt; }
    FURY_DECLARE_SERIALIZABLE_FIELDS(
      FURY_SERIALIZABLE_FIELD(1, &Curve::m_start_pnt),
      FURY_SERIALIZABLE_FIELD(2, &Curve::m_end_pnt)
    )
  protected:
    Vertex m_start_pnt;
    Vertex m_end_pnt;
  };
}
