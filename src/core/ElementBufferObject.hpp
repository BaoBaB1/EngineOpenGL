#pragma once

#include "OpenGLBuffer.hpp"

class ElementBufferObject : public OpenGLBuffer
{
public:
  OnlyMovable(ElementBufferObject)
  ElementBufferObject();
  ElementBufferObject(size_t size);
};
