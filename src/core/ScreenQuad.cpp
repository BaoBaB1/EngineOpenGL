#include "ScreenQuad.hpp"
#include "BindGuard.hpp"
#include "Shader.hpp"
#include "ShaderStorage.hpp"

static constexpr float quadVertices[] =
{ // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
  // positions   // texCoords
  -1.0f,  1.0f,  0.0f, 1.0f,
  -1.0f, -1.0f,  0.0f, 0.0f,
   1.0f, -1.0f,  1.0f, 0.0f,

  -1.0f,  1.0f,  0.0f, 1.0f,
   1.0f, -1.0f,  1.0f, 0.0f,
   1.0f,  1.0f,  1.0f, 1.0f
};

ScreenQuad::ScreenQuad()
{
  BindChainFIFO bind_chain({ &vao, &vbo });
  vao.link_attrib(0, 2, GL_FLOAT, sizeof(float) * 4, nullptr);
  vao.link_attrib(1, 2, GL_FLOAT, sizeof(float) * 4, (void*)(sizeof(float) * 2));
  vbo.set_data(quadVertices, sizeof(quadVertices), 0);
}

ScreenQuad::ScreenQuad(GLuint tex_id) : ScreenQuad()
{
  m_tex_id = tex_id;
}

void ScreenQuad::tick()
{
  // set GL_FILL mode because next we are rendering texture
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glDisable(GL_DEPTH_TEST);
  Shader& shader = ShaderStorage::get(ShaderStorage::ShaderType::SCREEN_QUAD);
  BindGuard<Shader> shader_bg(shader);
  BindChainFIFO chain({ &vao, &vbo });
  shader.set_int("screenTexture", 0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_tex_id);
  glDrawArrays(GL_TRIANGLES, 0, 6);
}
