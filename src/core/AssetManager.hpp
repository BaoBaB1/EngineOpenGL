#pragma once

#include <string>
#include <filesystem>
#include <unordered_set>
#include <optional>

namespace fury
{
  class AssetManager
  {
  public:
    static void init();
    static std::optional<std::filesystem::path> get_absolute_from_relative(const std::filesystem::path& asset);
    static std::optional<std::filesystem::path> get_relative_from_absolute(const std::filesystem::path& asset);
    static std::string add(const std::filesystem::path& asset, const std::string& folder = "");
    static const std::filesystem::path& get_assets_folder();
  private:
    static std::unordered_set<std::filesystem::path> assets;
  };
};
