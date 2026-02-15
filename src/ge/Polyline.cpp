#include "Polyline.hpp"
#include "./ge/Vertex.hpp"

namespace fury
{
  Polyline::Polyline()
  {
    emplace_mesh();
    m_render_config.use_indices = false;
    m_render_config.mode = GL_LINE_STRIP; 
    set_flag(HAS_SURFACE, false);
  }

  Polyline::Polyline(std::initializer_list<Vertex> points) : Polyline()
  {
    std::for_each(points.begin(), points.end(), [this](const Vertex& v) { add(v); });
  }

  Polyline::Polyline(const std::vector<Vertex>& points) : Polyline()
  {
    get_mesh(0).vertices() = points;
  }

  void Polyline::add(const Vertex& point)
  {
    get_mesh(0).append_vertex(point);
  }
}
