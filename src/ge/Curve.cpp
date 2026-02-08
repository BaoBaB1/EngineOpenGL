#include "Curve.hpp"

namespace fury
{
  Curve::Curve()
  {
    emplace_mesh();
    m_render_config.use_indices = false;
    m_render_config.mode = GL_LINE_STRIP;
    set_flag(HAS_SURFACE, false);
  }

  Curve::Curve(const Vertex& start, const Vertex& end) : Curve()
  {
    m_start_pnt = start;
    m_end_pnt = end;
  }
};
