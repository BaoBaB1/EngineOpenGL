#include "OpenGLBuffer.hpp"
#include "core/Logger.hpp"

namespace fury
{
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
      Logger::error("Could not set VBO data. Allocated size {}, trying to set {} at offset {}.", m_size, size_in_bytes,
                    offset);
      return;
    }
    glBufferSubData(m_type, offset, size_in_bytes, data);
  }

  void OpenGLBuffer::resize(size_t size_in_bytes)
  {
    glBufferData(m_type, size_in_bytes, nullptr, GL_STATIC_DRAW);
    m_size = size_in_bytes;
  }

  void OpenGLBuffer::resize_if_smaller(size_t size_in_bytes)
  {
    if (m_size < size_in_bytes)
    {
      resize(size_in_bytes);
    }
  }

  void OpenGLBuffer::bind() const
  {
    glBindBuffer(m_type, m_id);
  }

  void OpenGLBuffer::unbind() const
  {
    glBindBuffer(m_type, 0);
  }

  OpenGLBuffer::~OpenGLBuffer()
  {
    glDeleteBuffers(1, id_ref());
  }
} // namespace fury
