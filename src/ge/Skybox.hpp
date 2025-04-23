#pragma once

#include "core/opengl/Cubemap.hpp"
#include "core/opengl/VertexArrayObject.hpp"
#include "core/opengl/VertexBufferObject.hpp"

namespace fury
{
  class Skybox
  {
  public:
    Skybox();
    void set_cubemap(Cubemap&& cubemap) { m_cubemap = std::move(cubemap); }
    void render();
    const Cubemap& get_cubemap() const { return m_cubemap; }
  private:
    Cubemap m_cubemap;
    VertexBufferObject m_vbo;
    VertexArrayObject m_vao;
  };
}
