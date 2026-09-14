#include "Cubemap.hpp"
#include "core/opengl/Texture2D.hpp"

namespace fury
{
  void Cubemap::bind() const
  {
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_id);
  }

  void Cubemap::unbind() const
  {
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
  }

  void Cubemap::set_textures(const std::array<std::filesystem::path, 6>& textures)
  {
    bind();
    for (size_t i = 0; i < textures.size(); i++)
    {
      auto [pixels, image_data] = Texture2D::read_image(textures[i], false);
      const int format = (image_data.channels == 3 ? GL_RGB : GL_RGBA);
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<GLenum>(i), 0, format, image_data.width,
                   image_data.height, 0, format, GL_UNSIGNED_BYTE, pixels.get());
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    unbind();
  }
}; // namespace fury
