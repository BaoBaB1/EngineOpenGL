#pragma once

#include "Texture.hpp"
#include "utils/Utils.hpp"
#include <array>
#include <filesystem>

namespace fury
{
  class Cubemap : public Texture
  {
  public:
    Cubemap() = default;
    Cubemap(const std::array<std::string, 6>& textures);
    Cubemap(const std::array<std::filesystem::path, 6>& textures);
    template<typename T>
    void set_textures(const std::array<T, 6>& textures);
    void bind() const override;
    void unbind() const override;
    void resize(int w, int h) override {}
  };

  template<typename T>
  void Cubemap::set_textures(const std::array<T, 6>& textures)
  {
    bind();
    for (size_t i = 0; i < textures.size(); i++)
    {
      if constexpr (std::is_same_v<T, std::string>)
      {
        init(textures[i], false);
      }
      else if constexpr (std::is_same_v<T, std::filesystem::path>)
      {
        init(textures[i], false);
      }
      else
      {
        static_assert(utils::always_false<T>, "[Cubemap::set_textures] unsupported type");
      }
      const int format = (m_texture_data.channels == 3 ? GL_RGB : GL_RGBA);
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, m_texture_data.width, m_texture_data.height, 0,
                   format, GL_UNSIGNED_BYTE, m_texture_data.pixels.get());
      m_texture_data.pixels.reset();
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    unbind();
  }
}; // namespace fury
