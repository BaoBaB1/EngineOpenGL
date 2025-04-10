#pragma once

#include "opengl/VertexArrayObject.hpp"
#include "opengl/VertexBufferObject.hpp"
#include "ITickable.hpp"

namespace fury
{
  class ScreenQuad : public ITickable
  {
  public:
    ScreenQuad();
    ScreenQuad(GLuint tex_id);
    void tick() override;
    void set_texture_id(GLuint id) { m_tex_id = id; }
    GLuint get_texture_id() const { return m_tex_id; }
  private:
    VertexArrayObject vao;
    VertexBufferObject vbo;
    GLuint m_tex_id = 0;
  };
}
