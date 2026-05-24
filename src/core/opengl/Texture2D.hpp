#pragma once

#include "Texture.hpp"
#include "core/Macros.hpp"
#include <filesystem>

namespace fury
{
  class Texture2D : public Texture
  {
  public:
    FURY_REGISTER_BASE_CLASS_NO_DEFAULT_READ_WRITE_IMPL(Texture2D)
    static const Texture2D& get_placeholder();
    using Texture::init;
    Texture2D() = default;
    void init(int w, int h, int internal_format, int format, int pixel_type);
    void resize(int w, int h) override;
    void create();
    void bind() const override;
    void unbind() const override;
    int get_internal_format() const { return m_internal_fmt; }
    int get_format() const { return m_fmt; }
    int get_pixel_data_type() const { return m_pixel_data_type; }
  private:
    int m_internal_fmt = -1;
    int m_fmt = -1;
    int m_pixel_data_type = -1;
  };
}
