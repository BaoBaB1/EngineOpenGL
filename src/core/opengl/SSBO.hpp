#pragma once

#include "OpenGLBuffer.hpp"

namespace fury
{
  class SSBO : public OpenGLBuffer
  {
  public:
    SSBO();
    SSBO(size_t size);
    void set_binding_point(int binding_point);
    void bind_to_index();
  private:
    int m_binding_point = -1;
  };
} // namespace fury
