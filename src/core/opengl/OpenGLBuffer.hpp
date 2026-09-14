#pragma once

#include "OpenGLObject.hpp"
#include <glad/glad.h>

namespace fury
{
  class OpenGLBuffer : public OpenGLObject
  {
  public:
    FURY_OnlyMovable(OpenGLBuffer)
    OpenGLBuffer(int type);
    OpenGLBuffer(int type, size_t size);
    ~OpenGLBuffer();
    void resize(size_t size_in_bytes);
    void resize_if_smaller(size_t size_in_bytes);
    void set_data(const void* data, size_t size_in_bytes, size_t offset);
    void bind() const override;
    void unbind() const override;
    size_t get_size() const { return m_size; }
    int get_type() const { return m_type; }
  private:
    size_t m_size = 0;
    int m_type = -1; // GL_ARRAY_BUFFER | GL_ELEMENT_BUFFER ...
  };
} // namespace fury
