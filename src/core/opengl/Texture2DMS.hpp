#pragma once

#include "OpenGLObject.hpp"
#include "utils/Macro.hpp"

namespace fury
{
  class Texture2DMS : public OpenGLObject
  {
  public:
    OnlyMovable(Texture2DMS)
    Texture2DMS() = default;
    Texture2DMS(int w, int h, GLint format, int samples = 4);
    ~Texture2DMS();
    void init(int w, int h, GLint format, int samples);
    void bind() const override;
    void unbind() const override;
    int get_format() const { return m_format; }
    int get_samples() const { return m_samples; }
  private:
    int m_format = -1;
    int m_samples = 0;
  };
}
