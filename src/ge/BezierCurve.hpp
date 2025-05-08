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
    BezierCurve() = default;
    BezierCurve(BezierCurveType curve_type) : m_curve_type(curve_type) {}
    BezierCurve(BezierCurveType curve_type, const Vertex& start_pnt, const Vertex& end_pnt);
    uint32_t get_type() const override { return ObjectsRegistry::get_id<BezierCurve>(); }
    void set_control_points(const std::array<Vertex, 2>& c_points);
    std::pair<const Vertex*, int> get_control_points() const;
    BezierCurveType get_curve_type() const { return m_curve_type; }
  private:
    BezierCurveType m_curve_type = BezierCurveType::Quadratic;
    // 1 or 2
    std::array<Vertex, 2> m_control_points;
  };
}
