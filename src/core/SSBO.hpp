#pragma once

#include "OpenGLBuffer.hpp"

class SSBO : public OpenGLBuffer
{
public:
  OnlyMovable(SSBO)
  SSBO();
  SSBO(size_t size);
};
