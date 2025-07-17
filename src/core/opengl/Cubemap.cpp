#include "Cubemap.hpp"

namespace fury
{
  Cubemap::Cubemap(const std::array<std::string, 6>& textures)
  {
    set_textures(textures);
  }

  Cubemap::Cubemap(const std::array<std::string_view, 6>& textures)
  {
    set_textures(textures);
  }

  Cubemap::Cubemap(const std::array<std::filesystem::path, 6>& textures)
  {
    set_textures(textures);
  }

  void Cubemap::bind() const
  {
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_id);
  }

  void Cubemap::unbind() const
  {
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
  }
};
