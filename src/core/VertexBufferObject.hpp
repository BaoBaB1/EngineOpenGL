#pragma once

#include "OpenGLBuffer.hpp"

class VertexBufferObject : public OpenGLBuffer
{
public:
  OnlyMovable(VertexBufferObject)
  VertexBufferObject();
  VertexBufferObject(size_t size);
};