#include "Texture2D.hpp"
#include "core/AssetManager.hpp"
#include <stdexcept>
#include <memory>

namespace fury
{
  AssetHandle<Texture2D> Texture2D::get_placeholder()
  {
    static AssetHandle<Texture2D> tex = TextureManager::load(utils::get_textures_folder() / "placeholder.png");
    return tex;
  }

  std::pair<std::unique_ptr<unsigned char, StbDeleter>, ImageData>
  Texture2D::read_image(const std::filesystem::path& file, bool flip_vertically)
  {
    std::unique_ptr<unsigned char, StbDeleter> pixels;
    ImageData idata;
    stbi_set_flip_vertically_on_load(flip_vertically);
    idata.file = file;
    pixels.reset(stbi_load(file.string().c_str(), &idata.width, &idata.height, &idata.channels, 0));
    return { std::move(pixels), idata };
  }

  Texture2D::Texture2D(int w, int h, int internal_format, int format, int pixel_type)
  {
    create(w, h, internal_format, format, pixel_type);
  }

  Texture2D::Texture2D(const std::unique_ptr<unsigned char, StbDeleter>& pixels, const ImageData& image_data)
  {
    create(pixels, image_data);
  }

  Texture2D::Texture2D(const std::filesystem::path& file, bool flip_vertically)
  {
    create(file, flip_vertically);
  }

  Texture2D::~Texture2D()
  {
    glDeleteTextures(1, id_ref());
  }

  void Texture2D::bind() const
  {
    glBindTexture(GL_TEXTURE_2D, m_id);
  }

  void Texture2D::unbind() const
  {
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  void Texture2D::create(const std::unique_ptr<unsigned char, StbDeleter>& pixels, const ImageData& image_data)
  {
    if (!pixels) [[unlikely]]
    {
      Logger::error("[Texture2D::create] Pixels are empty.");
      return;
    }

    if (m_id != 0)
    {
      Logger::error("[Texture2D::create] Texture already created.");
      return;
    }

    glGenTextures(1, id_ref());
    static_cast<ImageData&>(m_texture_data) = image_data;

    m_texture_data.pixel_data_type = GL_UNSIGNED_BYTE;
    m_texture_data.internal_fmt = m_texture_data.fmt = (m_texture_data.channels == 3 ? GL_RGB : GL_RGBA);
    bind();
    glTexImage2D(GL_TEXTURE_2D, 0, m_texture_data.internal_fmt, m_texture_data.width, m_texture_data.height, 0,
                 m_texture_data.fmt, GL_UNSIGNED_BYTE, pixels.get());
    // texture wrapping around x,y axes
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // filtering methods
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, /*GL_LINEAR_MIPMAP_LINEAR*/ GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
    unbind();
  }

  void Texture2D::create(const std::filesystem::path& file, bool flip_vertically)
  {
    auto [pixels, image_data] = Texture2D::read_image(file, flip_vertically);
    create(std::move(pixels), image_data);
  }

  void Texture2D::resize(int w, int h)
  {
    m_texture_data.width = w;
    m_texture_data.height = h;
    bind();
    glTexImage2D(GL_TEXTURE_2D, 0, m_texture_data.internal_fmt, w, h, 0, m_texture_data.fmt,
                 m_texture_data.pixel_data_type, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, /*GL_NEAREST*/ GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, /*GL_NEAREST*/ GL_LINEAR);
    unbind();
  }

  void Texture2D::create(int w, int h, int internal_format, int format, int pixel_type)
  {
    if (m_id != 0)
    {
      Logger::error("[Texture2D::create] Texture already created.");
      return;
    }
    glGenTextures(1, id_ref());
    m_texture_data.fmt = format;
    m_texture_data.internal_fmt = internal_format;
    m_texture_data.pixel_data_type = pixel_type;
    m_texture_data.width = w;
    m_texture_data.height = h;
    resize(w, h);
  }

  uint32_t BinarySerializerSpec<Texture2D>::write(std::ofstream& ofs, const Texture2D* t)
  {
    uint32_t bytes_written = 0;
    bytes_written += BinarySerializer::write(ofs, &t->get_texture_data().file);
    return bytes_written;
  }

  uint32_t BinarySerializerSpec<Texture2D>::read(std::ifstream& ifs, Texture2D* t)
  {
    std::filesystem::path file;
    uint32_t bytes_read = BinarySerializer::read(ifs, &file);
    t->create(file);
    return bytes_read;
  }

} // namespace fury
