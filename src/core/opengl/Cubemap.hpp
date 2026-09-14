#pragma once

#include "OpenGLObject.hpp"
#include <filesystem>
#include <array>

namespace fury
{
  class Cubemap : public OpenGLObject
  {
  public:
    void set_textures(const std::array<std::filesystem::path, 6>& textures);
    void bind() const override;
    void unbind() const override;
  };
}; // namespace fury
