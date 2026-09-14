#include "UniformBuffer.hpp"
#include "core/Logger.hpp"

namespace fury
{
  UniformBuffer::UniformBuffer() : OpenGLBuffer(GL_UNIFORM_BUFFER)
  {
  }

  UniformBuffer::UniformBuffer(size_t size) : OpenGLBuffer(GL_UNIFORM_BUFFER, size)
  {
  }

  void UniformBuffer::set_binding_point(int binding_point)
  {
    m_binding_point = binding_point;
  }

  void UniformBuffer::bind_to_index()
  {
    if (m_binding_point == -1)
    {
      Logger::error("Trying to bind SSBO with invalid binding point.");
      return;
    }
    glBindBufferBase(GL_UNIFORM_BUFFER, m_binding_point, id());
  }
} // namespace fury
