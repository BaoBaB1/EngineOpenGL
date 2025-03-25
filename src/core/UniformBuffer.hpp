#pragma once

#include "OpenGLBuffer.hpp"

class UniformBuffer : public OpenGLBuffer
{
public:
  OnlyMovable(UniformBuffer)
  UniformBuffer();
  UniformBuffer(size_t size);
  void set_binding_point(int point) const;
};
