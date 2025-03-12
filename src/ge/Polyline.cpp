#include "Polyline.hpp"
#include "./ge/Vertex.hpp"

Polyline::Polyline()
{
  emplace_mesh();
  m_render_config.use_indices = false;
  m_render_config.mode = GL_LINE_STRIP;
}

void Polyline::add(const Vertex& point) 
{
  m_meshes[0].append_vertex(point);
}
