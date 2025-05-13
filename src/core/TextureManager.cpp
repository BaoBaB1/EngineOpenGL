#include "TextureManager.hpp"
#include "opengl/Texture2D.hpp"
#include <vector>

namespace fury
{
	std::map<std::filesystem::path, std::shared_ptr<Texture2D>> TextureManager::textures;

	std::shared_ptr<Texture2D> TextureManager::get(const std::filesystem::path& path)
	{
		auto it = textures.find(path);
		if (it != textures.end())
		{
			return it->second;
		}
		auto ret_val = textures.insert({path, std::make_shared<Texture2D>(path) });
		return ret_val.first->second;
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
}
