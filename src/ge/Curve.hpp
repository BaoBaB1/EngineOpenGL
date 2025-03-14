#pragma once

#include <vector>
#include "ge/Object3D.hpp"
#include "ge/Vertex.hpp"

class Curve : public Object3D {
public:
  Curve()
  {
    emplace_mesh();
    m_render_config.use_indices = false;
    m_render_config.mode = GL_LINE_STRIP;
  }
  Curve(const Vertex& start, const Vertex& end) : Curve()
  {
    m_start_pnt = start;
    m_end_pnt = end;
  }
  void set_start_point(const Vertex& start_pnt) { m_start_pnt = start_pnt; }
  void set_end_point(const Vertex& end_pnt) { m_end_pnt = end_pnt; }
  const Vertex& start_point() { return m_start_pnt; }
  const Vertex& end_point() { return m_end_pnt; }
  bool has_surface() const override { return false; }
  std::string name() const override { return "Curve"; }
protected:
  Vertex m_start_pnt; 
  Vertex m_end_pnt; 
  std::vector<Vertex> m_control_points;
};
