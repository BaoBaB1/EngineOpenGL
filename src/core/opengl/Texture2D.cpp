#include "Texture2D.hpp"
#include "core/AssetManager.hpp"

namespace fury
{
  const Texture2D& Texture2D::get_placeholder()
  {
    static Texture2D placeholder = Texture2D(AssetManager::get_absolute_from_relative("textures/placeholder.png").value());
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

  uint64_t Texture2D::write(std::ofstream& ofs) const
  {
    TextureType type = get_type();
    // path relative to assets folder
    const std::string path = AssetManager::get_relative_from_absolute(get_file()).value().string();
    const uint16_t path_len = path.size();
    ofs.write(reinterpret_cast<const char*>(&type), sizeof(TextureType));
    ofs.write(reinterpret_cast<const char*>(&path_len), sizeof(path_len));
    ofs.write(path.data(), path_len);
    return sizeof(TextureType) + sizeof(path_len) + path_len;
  }

  uint64_t Texture2D::read(std::ifstream& ifs)
  {
    TextureType type;
    uint16_t path_len;
    std::string path;
    ifs.read(reinterpret_cast<char*>(&type), sizeof(TextureType));
    ifs.read(reinterpret_cast<char*>(&path_len), sizeof(path_len));
    path.resize(path_len);
    ifs.read(path.data(), path_len);
    auto full_path = AssetManager::get_absolute_from_relative(path).value_or("");
    if (full_path.empty())
    {
      Logger::error("Failed to get absolute path from relative {} during Texture2D reading", path);
    }
    else
    {
      set_type(type);
      init(full_path.string());
    }
    // TODO: fixme. creates multiple texture instance for same file ...
    // texture and asset managers don't know about it???
    return sizeof(TextureType) + sizeof(path_len) + path_len;
  }

}
