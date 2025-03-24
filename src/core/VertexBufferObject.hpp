#pragma once

#include "OpenGLBuffer.hpp"
#include "ge/Vertex.hpp"

class VertexBufferObject : public OpenGLBuffer
{
public:
  constexpr static size_t DEFAULT_VBO_SIZE = sizeof(Vertex) * 50'000;
  OnlyMovable(VertexBufferObject)
  VertexBufferObject();
  VertexBufferObject(size_t size);
};