#include "TextureManager.hpp"
#include "opengl/Texture2D.hpp"
#include "Logger.hpp"
#include <vector>

namespace fury
{
  std::map<std::filesystem::path, std::shared_ptr<Texture2D>> TextureManager::textures;

  std::shared_ptr<Texture2D> TextureManager::get(const std::filesystem::path& path, bool create_if_not_exist)
  {
    if (auto it = textures.find(path); it != textures.end())
    {
      return it->second;
    }
    if (!create_if_not_exist)
    {
      return {};
    }
    auto tex = std::make_shared<Texture2D>();
    tex->init(path.string());
    tex->create();
    if (!tex->is_created())
    {
      Logger::info("Failed to create OpenGL texture from file {}. Its id is 0.", path.string());
      return {};
    }
    textures.insert({ path, tex });
    return tex;
  }

  bool TextureManager::add(const std::shared_ptr<Texture2D>& tex)
  {
    if (tex->get_file().empty())
    {
      Logger::info("[TextureManager::add]: Texture file is empty.");
      return false;
    }
    if (textures.contains(tex->get_file()))
    {
      Logger::info("[TextureManager::add]: Texture entry for file {} already exists.", tex->get_file());
      return false;
    }
    textures.insert({ tex->get_file(), tex });
    return true;
  }

  void TextureManager::remove_unused()
  {
    std::vector<decltype(textures)::iterator> to_remove;
    to_remove.reserve(4);
    for (decltype(textures)::iterator begin = textures.begin(), end = textures.end(); begin != end; ++begin)
    {
      if (begin->second.use_count() == 1)
      {
        to_remove.push_back(begin);
      }
    }
    for (const auto& it : to_remove)
    {
      textures.erase(it);
    }
  }
} // namespace fury
