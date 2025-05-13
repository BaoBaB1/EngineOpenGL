#include "Texture2D.hpp"
#include "core/AssetManager.hpp"

namespace fury
{
  const Texture2D& Texture2D::get_placeholder()
  {
    static Texture2D placeholder = Texture2D(AssetManager::get_from_relative("textures/placeholder.png").value());
    return placeholder;
  }

  Texture2D::Texture2D(int w, int h, GLint internalformat, GLint format, GLint type)
  {
    resize(w, h, internalformat, format, type);
  }

  Texture2D::Texture2D(const std::string& filename)
  {
    stbi_set_flip_vertically_on_load(true);
    init(filename);
  }

  Texture2D::Texture2D(const std::filesystem::path& file) : Texture2D(file.string())
  {
  }

  void Texture2D::resize(int w, int h, GLint internalformat, GLint format, GLint pixel_data_type)
  {
    m_height = h;
    m_width = w;
    m_internal_fmt = internalformat;
    m_fmt = format;
    m_pixel_data_type = pixel_data_type;
    bind();
    glTexImage2D(GL_TEXTURE_2D, 0, internalformat, w, h, 0, format, m_pixel_data_type, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, /*GL_NEAREST*/GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, /*GL_NEAREST*/GL_LINEAR);
    unbind();
  }

  void Texture2D::init(const std::string& filename)
  {
    auto data = Texture::load(filename.c_str());
    m_pixel_data_type = GL_UNSIGNED_BYTE;
    m_disabled = false;
    m_file = filename;
    m_internal_fmt = m_fmt = (m_nchannels == 3 ? GL_RGB : GL_RGBA);
    bind();
    glTexImage2D(GL_TEXTURE_2D, 0, m_internal_fmt, m_width, m_height, 0, m_fmt, GL_UNSIGNED_BYTE, data.get());
    // texture wrapping around x,y axes
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // filtering methods 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, /*GL_LINEAR_MIPMAP_LINEAR*/GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
    unbind();
  }

  void Texture2D::bind() const
  {
    glBindTexture(GL_TEXTURE_2D, m_id);
  }

  void Texture2D::unbind() const
  {
    glBindTexture(GL_TEXTURE_2D, 0);
  }
}
