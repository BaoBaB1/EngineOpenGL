#pragma once

#include <glad/glad.h>
#include <string>
#include <memory>
#include <stb_image.h>
#include "OpenGLObject.hpp"

struct StbDeleter
{
  void operator()(unsigned char* data) { free(data); }
};

enum class TextureType
{
  AMBIENT,
  DIFFUSE,
  SPECULAR,
  UNKNOWN,
  LAST = UNKNOWN
};

class Texture : public OpenGLObject
{
public:
  OnlyMovable(Texture)
  ~Texture();
  virtual void resize(int w, int h, GLint internalformat, GLint format, GLint type) = 0;
  virtual void resize(const std::string& filename) = 0;
  std::unique_ptr<unsigned char, StbDeleter> load(const char* filename);
  void disable() { m_disabled = true; }
  void enable() { m_disabled = false; }
  bool disabled() const { return m_disabled; }
  int width() const { return m_width; }
  int height() const { return m_height; }
  int nchannels() const { return m_nchannels; }
protected:
  Texture();
protected:
  int m_width = 0;
  int m_height = 0;
  int m_nchannels = 0;
  bool m_disabled = true;
};
