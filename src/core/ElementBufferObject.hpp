#pragma once

#include "OpenGLBuffer.hpp"

class ElementBufferObject : public OpenGLBuffer
{
public:
  constexpr static size_t DEFAULT_EBO_SIZE = sizeof(GLuint) * 40'000;
  OnlyMovable(ElementBufferObject)
  ElementBufferObject();
  ElementBufferObject(size_t size);
};
