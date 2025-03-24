#pragma once

#include "core/Cubemap.hpp"
#include "core/VertexArrayObject.hpp"
#include "core/VertexBufferObject.hpp"

class Skybox
{
public:
  Skybox() = default;
  Skybox(Cubemap&& cm);
  void set_cubemap(Cubemap&& cubemap) { m_cubemap = std::move(cubemap); }
  void render();
  const Cubemap& get_cubemap() const { return m_cubemap; }
private:
  Cubemap m_cubemap;
  VertexBufferObject m_vbo;
  VertexArrayObject m_vao;
};
