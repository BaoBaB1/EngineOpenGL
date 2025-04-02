#include "Texture.hpp"
#include <stdexcept>
#include <memory>

Texture::Texture()
{
  glGenTextures(1, id_ref());
}

Texture::~Texture()
{
  glDeleteTextures(1, id_ref());
}

std::unique_ptr<unsigned char, StbDeleter> Texture::load(const char* filename)
{
  StbDeleter deleter;
  std::unique_ptr<unsigned char, StbDeleter> data(stbi_load(filename, &m_width, &m_height, &m_nchannels, 0), deleter);
  if (!data)
  {
    throw std::runtime_error(std::string("Could not load texture from file ") + filename);
  }
  return data;
}
