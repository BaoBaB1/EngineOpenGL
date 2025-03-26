#pragma once

#include "OpenGLBuffer.hpp"

class UniformBuffer : public OpenGLBuffer
{
public:
  OnlyMovable(UniformBuffer)
  UniformBuffer();
  UniformBuffer(size_t size);
};
