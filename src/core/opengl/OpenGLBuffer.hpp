#pragma once

#include <glad/glad.h>
#include "OpenGLObject.hpp"

class OpenGLBuffer : public OpenGLObject
{
public:
  OnlyMovable(OpenGLBuffer)
  OpenGLBuffer(int type);
  OpenGLBuffer(int type, size_t size);
  ~OpenGLBuffer();
  void resize(size_t new_size);
  void resize_if_smaller(size_t new_size);
  void set_data(const void* data, size_t size_in_bytes, size_t offset);
  void bind() const override;
  void unbind() const override;
  size_t get_size() const { return m_size; }
  int get_type() const { return m_type; }
  void set_binding_point(int point) const;
private:
  size_t m_size = 0;
  int m_type = -1; // GL_ARRAY_BUFFER | GL_ELEMENT_BUFFER ...
};