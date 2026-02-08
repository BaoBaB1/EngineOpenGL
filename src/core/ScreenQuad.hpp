#pragma once

#include "opengl/VertexArrayObject.hpp"
#include "opengl/VertexBufferObject.hpp"
#include "ITickable.hpp"
#include <array>

namespace fury
{
  class ScreenQuad : public ITickable
  {
  public:
    ScreenQuad();
    void init(GLuint texture_id, bool is_single_channel = false);
    void init(const std::array<float, 24>& data, GLuint texture_id, bool is_single_channel = false);
    void tick(float dt) override;
    GLuint get_texture_id() const { return m_tex_id; }
  private:
    VertexArrayObject vao;
    VertexBufferObject vbo;
    GLuint m_tex_id = 0;
    bool m_is_single_channel = false;
  };
}
