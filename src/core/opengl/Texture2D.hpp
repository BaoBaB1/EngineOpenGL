#pragma once

#include "Texture.hpp"
#include "core/Macros.hpp"
#include <filesystem>

namespace fury
{
  class Texture2D : public Texture
  {
  public:
    FURY_REGISTER_CLASS_NO_DEFAULT_READ_WRITE_IMPL(Texture2D)
    static const Texture2D& get_placeholder();
    Texture2D() = default;
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
