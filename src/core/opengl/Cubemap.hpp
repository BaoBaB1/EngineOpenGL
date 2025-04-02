#pragma once

#include "Texture.hpp"
#include <array>

class Cubemap : public Texture
{
public:
  Cubemap() = default;
  OnlyMovable(Cubemap)
  Cubemap(const std::array<std::string, 6>& textures);
  Cubemap(const std::array<std::string_view, 6>& textures);
  template<typename T>
  void set_textures(const std::array<T, 6>& textures);
  void bind() const override;
  void unbind() const override;
  void resize(int w, int h, GLint internalformat, GLint format, GLint type) override {}
  void resize(const std::string& filename) override {}
};

template<typename T>
void Cubemap::set_textures(const std::array<T, 6>& textures)
{
  bind();
  stbi_set_flip_vertically_on_load(false);
  for (size_t i = 0; i < textures.size(); i++)
  {
    std::unique_ptr<unsigned char, StbDeleter> data;
    if constexpr (std::is_same_v<T, std::string>)
    {
      data = Texture::load(textures[i].c_str());
    }
    else if constexpr (std::is_same_v<T, std::string_view>)
    {
      data = Texture::load(textures[i].data());
    }
    else
    {
      data = Texture::load(static_cast<const char*>(textures[i]));
    }
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, m_width, m_height, 0, GL_RGB, GL_UNSIGNED_BYTE, data.get());
  }
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  unbind();
}
