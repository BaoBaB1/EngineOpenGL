#pragma once

#include "Common.hpp"
#include "AssetHandle.hpp"
#include "utils/Utils.hpp"
#include "Logger.hpp"
#include <string>
#include <filesystem>
#include <unordered_map>

namespace fury
{
  template<typename T>
  class AssetManager
  {
  public:
    static AssetHandle<T> create(const std::string& entry_name) { return store(entry_name, std::make_shared<T>()); }

    static AssetHandle<T> create()
    {
      static AssetId id = 0;
      id++;
      return store(id, std::make_shared<T>());
    }

    static AssetHandle<T> get_or_create(const std::string& lookup_name, const std::function<T()>& create_func)
    {
      const auto lookup_id = get_asset_id(lookup_name);
      if (auto handle = get_by_id(lookup_id))
      {
        return handle;
      }
      return store(lookup_id, std::make_shared<T>(create_func()));
    }

    static AssetHandle<T> load(const std::filesystem::path& path)
    {
      Logger::warn("AssetHandle load is empty for type {}.", utils::get_cls_name_helper<T>());
      return {};
    }

    static AssetHandle<T> get_by_name(const std::string& entry_name)
    {
      const AssetId id = get_asset_id(entry_name);
      const auto it = m_assets.find(id);
      if (it == m_assets.end())
      {
        Logger::trace("[AssetManager::get_by_name]: Asset with name {} does not exist.", entry_name);
        return {};
      }
      if (it->second.expired())
      {
        Logger::trace("[AssetManager::get_by_name]: Asset with name {} has expired.", entry_name);
        return {};
      }
      return { id, it->second.lock() };
    }

    static AssetHandle<T> get_by_id(AssetId id)
    {
      auto it = m_assets.find(id);
      if (it == m_assets.end())
      {
        Logger::trace("[AssetManager::get_by_id]: Asset with id {} does not exist.", id);
        return {};
      }
      if (it->second.expired())
      {
        Logger::trace("[AssetManager::get_by_id]: Asset with id {} has expired.", id);
        return {};
      }
      return { id, it->second.lock() };
    }

    static AssetHandle<T> store(const std::string& entry_name, const Ref<T>& asset)
    {
      const AssetId id = get_asset_id(entry_name);
      if (m_assets.contains(id))
      {
        Logger::warn("[AssetManager::store]: Asset with name {} already exists. Overriding it.", entry_name);
      }
      return store(id, asset);
    }

    static AssetHandle<T> store(AssetId id, const Ref<T>& asset)
    {
      const auto it = m_assets.find(id);
      if (it != m_assets.end() && !it->second.expired())
      {
        Logger::warn("[AssetManager::store]: Asset with id {} already exists. Overriding it.", id);
      }
      m_assets[id] = asset;
      return { id, asset };
    }

    static const std::unordered_map<AssetId, std::weak_ptr<T>>& get_assets() { return m_assets; }

    static void clear() { m_assets.clear(); }

  private:
    static AssetId get_asset_id(const std::filesystem::path& path) { return std::hash<std::filesystem::path> {}(path); }
    static AssetId get_asset_id(const std::string& name) { return std::hash<std::string> {}(name); }
  private:
    inline static std::unordered_map<AssetId, std::weak_ptr<T>> m_assets;
  };

  class Texture2D;
  template<>
  AssetHandle<Texture2D> AssetManager<Texture2D>::load(const std::filesystem::path& path);

}; // namespace fury
