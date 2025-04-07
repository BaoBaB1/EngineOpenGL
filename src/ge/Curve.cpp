#include "Curve.hpp"

namespace fury
{
  Curve::Curve() : Object3D("Curve")
  {
    emplace_mesh();
    m_render_config.use_indices = false;
    m_render_config.mode = GL_LINE_STRIP;
  }

  Curve::Curve(const Vertex& start, const Vertex& end) : Curve()
  {
    m_start_pnt = start;
    m_end_pnt = end;
  }
};
