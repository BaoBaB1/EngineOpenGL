#pragma once

#include "VertexArrayObject.hpp"
#include "VertexBufferObject.hpp"
#include "ElementBufferObject.hpp"

struct GPUBuffers 
{
  void bind_all();
  void unbind_all();

  VertexArrayObject vao;
  VertexBufferObject vbo;
  ElementBufferObject ebo;
};
