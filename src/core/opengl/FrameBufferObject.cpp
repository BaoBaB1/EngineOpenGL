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

  void FrameBufferObject::attach_renderbuffer(const RenderBufferCreateInfo& info)
  {
    m_info = info;
    if (m_render_buffer_id == 0)
    {
      glGenRenderbuffers(1, &m_render_buffer_id.id);
    }
    glBindRenderbuffer(GL_RENDERBUFFER, m_render_buffer_id);
    if (!info.is_multisampled)
    {
      glRenderbufferStorage(GL_RENDERBUFFER, info.format, info.w, info.h);
    }
    else
    {
      glRenderbufferStorageMultisample(GL_RENDERBUFFER, info.samples, info.format, info.w, info.h);
    }
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, info.attachment, GL_RENDERBUFFER, m_render_buffer_id);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
  }

  void FrameBufferObject::attach_texture(Texture2D&& tex, bool add_color_buffer)
  {
    m_texture = std::move(tex);
    attach_texture(GL_TEXTURE_2D, m_texture->id(), add_color_buffer);
  }

  void FrameBufferObject::attach_texture_ms(Texture2DMS&& tex, bool add_color_buffer)
  {
    m_texture_ms = std::move(tex);
    attach_texture(GL_TEXTURE_2D_MULTISAMPLE, m_texture_ms->id(), add_color_buffer);
  }

  void FrameBufferObject::attach_texture(int texture_type, int texture_id, bool add_color_buffer)
  {
    if (add_color_buffer)
    {
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture_type, texture_id, 0);
    }
    else
    {
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture_type, texture_id, 0);
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
