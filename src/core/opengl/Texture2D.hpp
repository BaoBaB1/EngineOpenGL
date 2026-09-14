#pragma once

#include "OpenGLObject.hpp"
#include "core/AssetHandle.hpp"
#include <memory>
#include <filesystem>
#include <stb_image.h>

namespace fury
{
  struct StbDeleter
  {
    void operator()(unsigned char* data) { free(data); }
  };

  struct ImageData
  {
    std::filesystem::path file;
    int width = 0;
    int height = 0;
    int channels = 0;
  };

  struct TextureData : ImageData
  {
    int internal_fmt = -1;
    int fmt = -1;
    int pixel_data_type = -1;
  };

  class Texture2D : public OpenGLObject
  {
  public:
    FURY_OnlyMovable(Texture2D)
    static AssetHandle<Texture2D> get_placeholder();
    static std::pair<std::unique_ptr<unsigned char, StbDeleter>, ImageData>
    read_image(const std::filesystem::path& file, bool flip_vertically = true);
    Texture2D() = default;
    Texture2D(int w, int h, int internal_format, int format, int pixel_type);
    Texture2D(const std::unique_ptr<unsigned char, StbDeleter>& pixels, const ImageData& image_data);
    Texture2D(const std::filesystem::path& file, bool flip_vertically = true);
    ~Texture2D();
    void create(int w, int h, int internal_format, int format, int pixel_type);
    void create(const std::unique_ptr<unsigned char, StbDeleter>& pixels, const ImageData& image_data);
    void create(const std::filesystem::path& file, bool flip_vertically = true);
    void resize(int w, int h);
    const TextureData& get_texture_data() const { return m_texture_data; }
    void bind() const override;
    void unbind() const override;
  private:
    TextureData m_texture_data;
  };

  template<>
  struct BinarySerializerSpec<Texture2D>
  {
    static uint32_t write(std::ofstream& ofs, const Texture2D* t);
    static uint32_t read(std::ifstream& ifs, Texture2D* t);
  };

} // namespace fury
