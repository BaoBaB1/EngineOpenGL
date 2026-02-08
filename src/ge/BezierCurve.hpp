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
    FURY_REGISTER_DERIVED_CLASS(BezierCurve, Curve)
    BezierCurve() = default;
    BezierCurve(BezierCurveType curve_type) : m_curve_type(curve_type) {}
    BezierCurve(BezierCurveType curve_type, const Vertex& start_pnt, const Vertex& end_pnt);
    void set_control_points(const std::array<Vertex, 2>& c_points);
    std::pair<const Vertex*, int> get_control_points() const;
    BezierCurveType get_curve_type() const { return m_curve_type; }
    FURY_DECLARE_SERIALIZABLE_FIELDS(
      FURY_SERIALIZABLE_FIELD(1, &BezierCurve::m_curve_type),
      FURY_SERIALIZABLE_FIELD(2, &BezierCurve::m_control_points)
    )
  private:
    BezierCurveType m_curve_type = BezierCurveType::Quadratic;
    // 1 or 2
    std::array<Vertex, 2> m_control_points;
  };
}
