#pragma once

#include <filesystem>
#include <map>
#include <memory>

namespace fury
{
  class Texture2D;

  class TextureManager
  {
  public:
    static std::shared_ptr<Texture2D> get(const std::filesystem::path& path);
    static void remove_unused();
  private:
    static std::map<std::filesystem::path, std::shared_ptr<Texture2D>> textures;
  };
}
