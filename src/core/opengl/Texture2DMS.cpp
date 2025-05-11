#include "Texture2DMS.hpp"
#include "core/Logger.hpp"

namespace fury
{
  Texture2DMS::Texture2DMS(int w, int h, GLint format, int samples)
  {
    init(w, h, format, samples);
  }

  Texture2DMS::~Texture2DMS()
  {
    glDeleteTextures(1, &m_id.id);
  }

  void Texture2DMS::init(int w, int h, GLint format, int samples)
  {
    if (id())
    {
      Logger::error("Could not init multisample texture since it's already been initialized!");
      return;
    }
    m_format = format;
    m_samples = samples;
    glGenTextures(1, &m_id.id);
    bind();
    // this is immutable
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, format, w, h, GL_TRUE);
    unbind();
  }

  void Texture2DMS::bind() const
  {
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_id);
  }

  void Texture2DMS::unbind() const
  {
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
  }
}
