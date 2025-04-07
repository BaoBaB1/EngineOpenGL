#pragma once

#include "ge/Curve.hpp"
#include <array>

namespace fury
{
  enum class BezierCurveType
  {
    Quadratic = 1,
    Cubic
  };

  class BezierCurve : public Curve
  {
  public:

  public:
    BezierCurve(BezierCurveType type) : m_type(type) {}
    BezierCurve(BezierCurveType type, const Vertex& start_pnt, const Vertex& end_pnt);
    void set_control_points(const std::array<Vertex, 2>& c_points);
    std::pair<const Vertex*, int> get_control_points() const;
    BezierCurveType type() const { return m_type; }
  private:
    BezierCurveType m_type = BezierCurveType::Quadratic;
    // 1 or 2
    std::array<Vertex, 2> m_control_points;
  };
}
