#include "OpenGLBuffer.hpp"
#include "core/Logger.hpp"

OpenGLBuffer::OpenGLBuffer(int type) : m_type(type)
{
  glGenBuffers(1, id_ref());
}

OpenGLBuffer::OpenGLBuffer(int type, size_t size) : m_type(type)
{
  glGenBuffers(1, id_ref());
  resize(size);
}

void OpenGLBuffer::set_data(const void* data, size_t size_in_bytes, size_t offset)
{
  if (offset + size_in_bytes > m_size)
  {
    Logger::error("Could not set VBO data. offset + size_in_bytes >= m_size");
    return;
  }
  glBufferSubData(m_type, offset, size_in_bytes, data);
}


void OpenGLBuffer::resize(size_t new_size)
{
  bind();
  glBufferData(m_type, new_size, nullptr, GL_STATIC_DRAW);
  m_size = new_size;
  unbind();
}

void OpenGLBuffer::resize_if_smaller(size_t new_size)
{
  if (m_size < new_size)
    resize(new_size);
}

void OpenGLBuffer::bind() const
{
  glBindBuffer(m_type, m_id);
}

void OpenGLBuffer::unbind() const
{
  glBindBuffer(m_type, 0);
}

void OpenGLBuffer::set_binding_point(int point) const
{
  glBindBufferBase(m_type, point, id());
}

OpenGLBuffer::~OpenGLBuffer()
{
  glDeleteBuffers(1, id_ref());
}
