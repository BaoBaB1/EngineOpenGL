#pragma once

#include "ge/Curve.hpp"

namespace fury
{
  class BezierCurve : public Curve
  {
  public:
    enum class Type
    {
      Quadratic = 1,
      Cubic,
    };
  public:
    BezierCurve(Type type) : m_type(type) {}
    BezierCurve(Type type, const Vertex& start_pnt, const Vertex& end_pnt);
    void set_control_points(const std::vector<Vertex>& c_points);
    std::vector<Vertex>& control_points() { return m_control_points; }
    const std::vector<Vertex>& control_points() const { return m_control_points; }
    Type type() const { return m_type; }
  private:
    Type m_type;
  };
}
