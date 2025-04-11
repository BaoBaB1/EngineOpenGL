#pragma once

#include "Texture.hpp"
#include <filesystem>

namespace fury
{
  class Texture2D : public Texture
  {
  public:
    OnlyMovable(Texture2D)
      Texture2D(int w, int h, GLint internalformat, GLint format, GLint type);
    Texture2D(const std::string&);
    Texture2D(const std::filesystem::path& file);
    void resize(int w, int h, GLint internalformat, GLint format, GLint type) override;
    void init(const std::string& filename) override;
    void bind() const override;
    void unbind() const override;
    int internal_fmt() const { return m_internal_fmt; }
    int format() const { return m_fmt; }
    int pixel_data_type() const { return m_pixel_data_type; }
  private:
    int m_internal_fmt = -1;
    int m_fmt = -1;
    int m_pixel_data_type = -1;
  };
}
