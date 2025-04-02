#include "BezierCurve.hpp"

namespace fury
{
  BezierCurve::BezierCurve(Type type, const Vertex& start_pnt, const Vertex& end_pnt)
    : Curve(start_pnt, end_pnt)
  {
    m_type = type;
  }

  void BezierCurve::set_control_points(const std::vector<Vertex>& c_points)
  {
    // TODO: instead of throwing exception make class like ErrorDialog or maybe return error code? 
    switch (m_type)
    {
    case BezierCurve::Type::Quadratic:
      if (c_points.size() != 1)
        throw std::exception("For quadratic Bezier curve n of control points must be 1");
      break;
    case BezierCurve::Type::Cubic:
      if (c_points.size() != 2)
        throw std::exception("For qubic Bezier curve n of control points must be 2");
      break;
    default:
      throw std::exception("Unsupported Bezier curve type");
      break;
    }

    Mesh& mesh = get_mesh(0);
    m_control_points = c_points;
    mesh.vertices().clear();
    mesh.faces().clear();

    constexpr float step = 0.005f;
    std::vector<Vertex>& vertices = mesh.vertices();
    vertices.reserve(static_cast<size_t>(1. / step));

    if (m_type == BezierCurve::Type::Quadratic)
    {
      // B(t) = (1-t)^2 * P0 + 2t(1-t) * P1 + t^2 * P2 , where 
      // P0 - start point 
      // P1 - control point
      // P2 - end point
      // 0 <= t <= 1
      for (float t = 0.0; t <= 1.0; t += step)
      {
        Vertex res = ((1 - t) * (1 - t) * m_start_pnt) +
          (2 * t * (1 - t) * m_control_points[0]) +
          (t * t * m_end_pnt);
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
      const Vertex& P0 = m_start_pnt;
      const Vertex& P1 = m_control_points[0];
      const Vertex& P2 = m_control_points[1];
      const Vertex& P3 = m_end_pnt;
      for (float t = 0.0; t <= 1.0; t += step)
      {
        Vertex res =
          (std::pow((1 - t), 3) * P0) +
          (3 * std::pow((1 - t), 2) * t * P1) +
          (3 * (1 - t) * t * t * P2) +
          (std::pow(t, 3) * P3);
        res.color = m_color;
        vertices.push_back(res);
      }
    }
  }
}
