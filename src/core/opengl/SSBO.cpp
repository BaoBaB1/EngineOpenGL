#include "SSBO.hpp"
#include "core/Logger.hpp"

namespace fury
{
  SSBO::SSBO() : OpenGLBuffer(GL_SHADER_STORAGE_BUFFER)
  {
  }

  SSBO::SSBO(size_t size) : OpenGLBuffer(GL_SHADER_STORAGE_BUFFER, size)
  {
  }

  void SSBO::set_binding_point(int binding_point)
  {
    m_binding_point = binding_point;
  }

  void SSBO::bind_to_index()
  {
    if (m_binding_point == -1)
    {
      Logger::error("Trying to bind SSBO with invalid binding point.");
      return;
    }
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_binding_point, id());
  }

} // namespace fury
