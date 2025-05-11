#pragma once

#include "Texture2D.hpp"
#include "Texture2DMS.hpp"
#include <glad/glad.h>
#include <optional>

namespace fury
{
  struct RenderBufferCreateInfo
  {
    GLsizei w = 0;
    GLsizei h = 0;
    GLenum format = GL_DEPTH24_STENCIL8;
    GLenum attachment = GL_DEPTH_STENCIL_ATTACHMENT;
    bool is_multisampled = false;
    int samples = 4;
  };

  class FrameBufferObject : public OpenGLObject
  {
  public:
    OnlyMovable(FrameBufferObject)
    FrameBufferObject();
    ~FrameBufferObject();
    void attach_renderbuffer(const RenderBufferCreateInfo& info);
    void attach_texture(Texture2D&& tex, bool add_color_buffer = true);
    void attach_texture_ms(Texture2DMS&& tex, bool add_color_buffer = true);
    void bind() const override;
    void unbind() const override;
    bool is_complete() const;
    const RenderBufferCreateInfo& get_info() const { return m_info; }
    std::optional<Texture2D>& texture() { return m_texture; }
    const std::optional<Texture2D>& texture() const { return m_texture; }
    std::optional<Texture2DMS>& texture_ms() { return m_texture_ms; }
    const std::optional<Texture2DMS>& texture_ms() const { return m_texture_ms; }
  private:
    void attach_texture(int texture_type, int texture_id, bool add_color_buffer);
  private:
    OpenGLIdWrapper<GLuint> m_render_buffer_id;
    RenderBufferCreateInfo m_info;
    std::optional<Texture2D> m_texture;
    std::optional<Texture2DMS> m_texture_ms;
  };
}
