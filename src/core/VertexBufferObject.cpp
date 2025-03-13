#include "VertexBufferObject.hpp"

VertexBufferObject::VertexBufferObject()
{
  glGenBuffers(1, id_ref());
}

void VertexBufferObject::set_data(const void* vertices, size_t size_in_bytes)
{
  if (m_size == 0 || m_size < size_in_bytes)
  {
    glBufferData(GL_ARRAY_BUFFER, size_in_bytes, vertices, GL_STATIC_DRAW);
  }
  else
  {
    glBufferSubData(GL_ARRAY_BUFFER, 0, size_in_bytes, vertices);
  }
  m_size = std::max(m_size, size_in_bytes);
}

void VertexBufferObject::bind() const
{
  glBindBuffer(GL_ARRAY_BUFFER, m_id);
}

void VertexBufferObject::unbind() const
{
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

VertexBufferObject::~VertexBufferObject()
{
  glDeleteBuffers(1, id_ref());
}
