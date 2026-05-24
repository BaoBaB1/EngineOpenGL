#pragma once

#include <filesystem>
#include <map>
#include <memory>

namespace fury
{
  class Texture2D;
  struct TextureData;

  class TextureManager
  {
  public:
    static std::shared_ptr<Texture2D> get(const std::filesystem::path& path, bool create_if_not_exist = true);
    static bool add(const std::shared_ptr<Texture2D>& tex);
    static void remove_unused();
  private:
    static std::map<std::filesystem::path, std::shared_ptr<Texture2D>> textures;
  };
}
