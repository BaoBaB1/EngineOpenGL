#include "Texture2D.hpp"
#include "core/AssetManager.hpp"

namespace fury
{
  const Texture2D& Texture2D::get_placeholder()
  {
    static Texture2D placeholder;
    static bool once = false;
    if (!once)
    {
      placeholder.init(AssetManager::get_absolute_from_relative("textures/placeholder.png").value());
      placeholder.create();
      once = true;
    }
    return placeholder;
  }

  void Texture2D::create()
  {
    if (!is_initialized())
    {
      Logger::error("[Texture2D::create]: Tried to create uninitialized texture.");
      return;
    }
    else
    {
      // This can happen if texture was created and initialized in other thread
      if (m_id == 0)
      {
        glGenTextures(1, id_ref());
        if (m_id == 0)
        {
          Logger::error("[Texture2D::create] Texture id is 0.");
          return;
        }
      }
      m_pixel_data_type = GL_UNSIGNED_BYTE;
      m_disabled = false;
      m_internal_fmt = m_fmt = (m_texture_data.channels == 3 ? GL_RGB : GL_RGBA);
      bind();
      glTexImage2D(GL_TEXTURE_2D, 0, m_internal_fmt, m_texture_data.width, m_texture_data.height, 0, m_fmt,
                   GL_UNSIGNED_BYTE, m_texture_data.pixels.get());
      // texture wrapping around x,y axes
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      // filtering methods
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, /*GL_LINEAR_MIPMAP_LINEAR*/ GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glGenerateMipmap(GL_TEXTURE_2D);
      unbind();
      // No more holding pixels in RAM
      m_texture_data.pixels.reset();
    }
  }

  void Texture2D::init(int w, int h, int internal_format, int format, int pixel_type)
  {
    m_fmt = format;
    m_internal_fmt = internal_format;
    m_pixel_data_type = pixel_type;
    m_texture_data.width = w;
    m_texture_data.height = h;
  }

  void Texture2D::resize(int w, int h)
  {
    m_texture_data.width = w;
    m_texture_data.height = h;
    bind();
    glTexImage2D(GL_TEXTURE_2D, 0, m_internal_fmt, w, h, 0, m_fmt, m_pixel_data_type, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, /*GL_NEAREST*/ GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, /*GL_NEAREST*/ GL_LINEAR);
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
      create();
    }
    // TODO: fixme. creates multiple texture instance for same file ...
    // texture and asset managers don't know about it???
    return sizeof(TextureType) + sizeof(path_len) + path_len;
  }

} // namespace fury
