#include "Polyline.hpp"
#include "./ge/Vertex.hpp"

Polyline::Polyline() : Object3D("Polyline")
{
  emplace_mesh();
  m_render_config.use_indices = false;
  m_render_config.mode = GL_LINE_STRIP;
}

void Polyline::add(const Vertex& point) 
{
  get_mesh(0).append_vertex(point);
}
