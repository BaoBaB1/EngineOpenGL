#include "BezierCurve.hpp"

namespace fury
{
  BezierCurve::BezierCurve(BezierCurveType curve_type, const Vertex& start_pnt, const Vertex& end_pnt)
    : Curve(start_pnt, end_pnt)
  {
    m_curve_type = curve_type;
  }

  void BezierCurve::set_control_points(const std::array<Vertex, 2>& c_points)
  {
    m_control_points = c_points;
    Mesh& mesh = get_mesh(0);
    mesh.vertices().clear();
    mesh.faces().clear();

    constexpr float step = 0.005f;
    std::vector<Vertex>& vertices = mesh.vertices();
    vertices.reserve(static_cast<size_t>(1. / step));

    if (m_curve_type == BezierCurveType::Quadratic)
    {
      // B(t) = (1-t)^2 * P0 + 2t(1-t) * P1 + t^2 * P2 , where 
      // P0 - start point 
      // P1 - control point
      // P2 - end point
      // 0 <= t <= 1
      for (float t = 0.0; t <= 1.0; t += step)
      {
        Vertex res = ((1 - t) * (1 - t) * m_start_pnt.position) +
          (2 * t * (1 - t) * m_control_points[0].position) +
          (t * t * m_end_pnt.position);
        res.color = m_color;
        vertices.push_back(res);
      }
    }
    else
    {
      // B(t) = ( (1-t)^3 * P0 ) + ( 3(1-t)^2 * t * P1 ) + ( 3(1-t) * t^2 * P2 ) + (t^3 * P3), where 
      // P0 - start point 
      // P1 - control point
      // P2 - control point
      // P3 - end point
      // 0 <= t <= 1
      const glm::vec3& P0 = m_start_pnt.position;
      const glm::vec3& P1 = m_control_points[0].position;
      const glm::vec3& P2 = m_control_points[1].position;
      const glm::vec3& P3 = m_end_pnt.position;
      for (float t = 0.0; t <= 1.0; t += step)
      {
        Vertex res =
          (std::pow((1 - t), 3.f) * P0) +
          (3 * std::pow((1 - t), 2.f) * t * P1) +
          (3 * (1 - t) * t * t * P2) +
          (std::pow(t, 3.f) * P3);
        res.color = m_color;
        vertices.push_back(res);
      }
    }
  }

  std::pair<const Vertex*, int> BezierCurve::get_control_points() const
  {
    if (m_curve_type == BezierCurveType::Quadratic)
      return { &m_control_points[0], 1 };
    return { &m_control_points[0], 2 };
  }
}
