#pragma once

#include "OpenGLObject.hpp"
#include <glad/glad.h>
#include <string>
#include <memory>
#include <stb_image.h>

namespace fury
{
  struct StbDeleter
  {
    void operator()(unsigned char* data) { free(data); }
  };

  enum class TextureType : uint8_t
  {
    AMBIENT,
    DIFFUSE,
    SPECULAR,
    LAST
  };

  struct TextureData
  {
    std::string file;
    std::unique_ptr<unsigned char, StbDeleter> pixels;
    int width = 0;
    int height = 0;
    int channels = 0;
  };

  class Texture : public OpenGLObject
  {
  public:
    FURY_OnlyMovable(Texture)
    ~Texture();
    virtual void resize(int w, int h) = 0;
    void init(const std::filesystem::path& file, bool flip_vertically = true);
    void init(const std::string& file, bool flip_vertically = true);
    void disable() { m_disabled = true; }
    void enable() { m_disabled = false; }
    bool disabled() const { return m_disabled; }
    bool is_initialized() const { return m_texture_data.pixels != nullptr; }
    bool is_created() const { return id() > 0; }
    int width() const { return m_texture_data.width; }
    int height() const { return m_texture_data.height; }
    int nchannels() const { return m_texture_data.channels; }
    void set_type(TextureType type) { m_type = type; }
    TextureType get_type() const { return m_type; }
    const std::string& get_file() const { return m_texture_data.file; }
  protected:
    Texture();
  protected:
    TextureData m_texture_data;
    bool m_disabled = true;
    TextureType m_type = TextureType::LAST;
  };
}
