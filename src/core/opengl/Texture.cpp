#include "Texture.hpp"
#include <stdexcept>
#include <memory>

namespace fury
{
  Texture::Texture()
  {
    glGenTextures(1, id_ref());
  }

  Texture::~Texture()
  {
    glDeleteTextures(1, id_ref());
  }

  void Texture::init(const std::filesystem::path& file, bool flip_vertically)
  {
    init(file.string(), flip_vertically);
  }

  void Texture::init(const std::string& file, bool flip_vertically)
  {
    stbi_set_flip_vertically_on_load(flip_vertically);
    m_texture_data.file = file;
    m_texture_data.pixels.reset(
        stbi_load(file.c_str(), &m_texture_data.width, &m_texture_data.height, &m_texture_data.channels, 0));
  }
}
