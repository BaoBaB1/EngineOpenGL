#include "AssetManager.hpp"
#include "utils/Utils.hpp"
#include "Logger.hpp"

namespace fury
{
	const static std::string assets_folder_generic = AssetManager::get_assets_folder().generic_string();

	std::unordered_set<std::string> AssetManager::assets;

	void AssetManager::init()
	{
		assets.clear();
		assets.reserve(32);
		for (const auto& entry : std::filesystem::recursive_directory_iterator(assets_folder_generic))
		{
			if (!entry.is_directory())
			{
				assets.insert(get_from_absolute(entry.path()).value());
			}
		}
	}

	// transforms relative path into absolute within assets directory
	std::optional<std::filesystem::path> AssetManager::get_from_relative(const std::string& asset)
	{
		auto it = assets.find(asset);
		return it != assets.end() ? (std::filesystem::path(assets_folder_generic) / *it) : std::optional<std::filesystem::path>{};
	}

	// transforms absolute path into relative within assets directory
	std::optional<std::string> AssetManager::get_from_absolute(const std::filesystem::path& asset)
	{
		std::string generic_path = asset.generic_string();
		if (generic_path.find(assets_folder_generic) != std::string::npos)
		{
			return { generic_path.erase(0, assets_folder_generic.size() + 1) };
		}
		return {};
	}

	std::string AssetManager::add(const std::filesystem::path& asset, const std::string& folder)
	{
		const std::string relative_path_entry = "custom/" + folder + "/" + asset.filename().string();
		// if we already have asset either in custom folder or it's a preexisting one
		if (assets.find(relative_path_entry) != assets.end() || get_from_absolute(asset))
		{
			return relative_path_entry;
		}

		std::filesystem::path copy_to = get_assets_folder() / "custom";
		if (!folder.empty())
		{
			copy_to /= folder;
			std::filesystem::create_directories(copy_to);
		}
		std::filesystem::copy(asset, copy_to / asset.filename(), std::filesystem::copy_options::skip_existing /*overwrite_existing*/);
		assets.insert(relative_path_entry);
		return relative_path_entry;
	}

	const std::filesystem::path& AssetManager::get_assets_folder()
	{
		static std::filesystem::path folder = utils::get_project_root_dir() / "assets";
		return folder;
	}
}
