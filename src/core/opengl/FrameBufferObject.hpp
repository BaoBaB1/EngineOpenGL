#pragma once

#include <glad/glad.h>
#include <optional>
#include "Texture2D.hpp"
#include "core/Shader.hpp"

class FrameBufferObject : public OpenGLObject
{
public:
  OnlyMovable(FrameBufferObject)
  FrameBufferObject();
  ~FrameBufferObject();
  void attach_renderbuffer(int w, int h, GLenum internalformat, GLenum attachment);
  void attach_texture(int w, int h, GLint internalformat, GLint format, GLint type);
  void attach_texture(Texture2D&& tex);
  void bind() const override;
  void unbind() const override;
  bool is_complete() const;
  int rb_internal_format() const { return m_rb_internal_fmt; }
  int rb_attachment() const { return m_rb_attachment; }
  std::optional<Texture2D>& texture() { return m_texture; }
  const std::optional<Texture2D>& texture() const { return m_texture; }
private:
  OpenGLIdWrapper<GLuint> m_render_buffer_id;
  int m_rb_internal_fmt = -1;
  int m_rb_attachment = -1;
  std::optional<Texture2D> m_texture;
};
