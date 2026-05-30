#include "ScreenQuad.hpp"
#include "BindGuard.hpp"
#include "Shader.hpp"
#include "ShaderStorage.hpp"

// clang-format off
static constexpr std::array screen_quad_vertices =
{ // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
  // positions   // texCoords
  -1.0f,  1.0f,  0.0f, 1.0f,
  -1.0f, -1.0f,  0.0f, 0.0f,
   1.0f, -1.0f,  1.0f, 0.0f,

  -1.0f,  1.0f,  0.0f, 1.0f,
   1.0f, -1.0f,  1.0f, 0.0f,
   1.0f,  1.0f,  1.0f, 1.0f
};
// clang-format on

namespace fury
{
  ScreenQuad::ScreenQuad()
  {
    BindChainFIFO bind_chain({ &m_vao, &m_vbo });
    m_vao.link_attrib(0, 2, GL_FLOAT, sizeof(float) * 4, nullptr);
    m_vao.link_attrib(1, 2, GL_FLOAT, sizeof(float) * 4, (void*)(sizeof(float) * 2));
    m_vbo.resize(::screen_quad_vertices.size() * sizeof(float));
  }

  void ScreenQuad::init(GLuint texture_id, bool is_single_channel)
  {
    init(::screen_quad_vertices, texture_id, is_single_channel);
  }

  void ScreenQuad::init(const std::array<float, 24>& data, GLuint texture_id, bool is_single_channel)
  {
    m_vbo.bind();
    m_vbo.set_data(data.data(), data.size() * sizeof(float), 0);
    m_vbo.unbind();
    m_tex_id = texture_id;
    m_is_single_channel = is_single_channel;
  }

  void ScreenQuad::tick(float)
  {
    // set GL_FILL mode because next we are rendering texture
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_DEPTH_TEST);
    Shader& shader = ShaderStorage::get(ShaderStorage::ShaderType::SCREEN_QUAD);
    BindGuard shader_bg(shader);
    BindGuard bg(m_vao);
    shader.set_int("screenTexture", 0);
    shader.set_bool("isSingleChannel", m_is_single_channel);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_tex_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
  }
} // namespace fury
