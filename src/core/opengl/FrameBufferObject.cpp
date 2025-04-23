#include "FrameBufferObject.hpp"

namespace fury
{
  FrameBufferObject::FrameBufferObject()
  {
    glGenFramebuffers(1, id_ref());
  }

  void FrameBufferObject::bind() const
  {
    glBindFramebuffer(GL_FRAMEBUFFER, m_id);
  }

  void FrameBufferObject::unbind() const
  {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  bool FrameBufferObject::is_complete() const
  {
    return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
  }

  void FrameBufferObject::attach_renderbuffer(int w, int h, GLenum internalformat, GLenum attachment)
  {
    m_rb_internal_fmt = internalformat;
    m_rb_attachment = attachment;
    if (m_render_buffer_id == 0)
    {
      glGenRenderbuffers(1, &m_render_buffer_id.id);
    }
    glBindRenderbuffer(GL_RENDERBUFFER, m_render_buffer_id);
    glRenderbufferStorage(GL_RENDERBUFFER, internalformat, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, m_render_buffer_id);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
  }

  void FrameBufferObject::attach_texture(int w, int h, GLint internalformat, GLint format, GLint type, bool add_color_buffer)
  {
    attach_texture(Texture2D(w, h, internalformat, format, type), add_color_buffer);
  }

  void FrameBufferObject::attach_texture(Texture2D&& tex, bool add_color_buffer)
  {
    m_texture = std::move(tex);
    if (add_color_buffer)
    {
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture->id(), 0);
    }
    else
    {
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_texture->id(), 0);
      glDrawBuffer(GL_NONE);
      glReadBuffer(GL_NONE);
    }
  }

  FrameBufferObject::~FrameBufferObject()
  {
    glDeleteFramebuffers(1, id_ref());
    glDeleteRenderbuffers(1, &m_render_buffer_id.id);
  }
}
