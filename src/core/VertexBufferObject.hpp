#pragma once

#include <glad/glad.h>
#include "OpenGLObject.hpp"

class VertexBufferObject : public OpenGLObject
{
public:
  OnlyMovable(VertexBufferObject)
  VertexBufferObject();
  ~VertexBufferObject();
  void set_data(const void* vertices, size_t size_in_bytes);
  void bind() const override;
  void unbind() const override;
private:
  size_t m_size = 0;
};