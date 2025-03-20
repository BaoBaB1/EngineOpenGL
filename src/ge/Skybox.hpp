#pragma once

#include "core/Cubemap.hpp"
#include "core/VertexArrayObject.hpp"
#include "core/VertexBufferObject.hpp"

class Skybox
{
public:
  Skybox(Cubemap&& cm);
  void render() const;
  const Cubemap& get_cubemap() const { return m_cubemap; }
private:
  Cubemap m_cubemap;
  VertexBufferObject m_skybox_vbo;
  VertexArrayObject m_skybox_vao;
};
