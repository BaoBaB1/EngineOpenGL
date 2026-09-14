#pragma once

#include "core/TypeTraits.hpp"
#include "core/BoundingBox.hpp"
#include <filesystem>
#include <string>
#include <vector>
#include <string_view>
#include <functional>
#include <glm/glm.hpp>

namespace fury
{
  struct MeshNode;

  namespace utils
  {
    class ScopeGuard
    {
    public:
      ScopeGuard() = default;
      ScopeGuard(std::function<void()> func) { set_function(func); }
      ScopeGuard(const ScopeGuard&) = delete;
      ScopeGuard& operator=(const ScopeGuard&) = delete;
      ScopeGuard(ScopeGuard&&) noexcept = default;
      ScopeGuard& operator=(ScopeGuard&&) noexcept = default;
      void set_function(std::function<void()> func) { m_func = std::move(func); }
      ~ScopeGuard()
      {
        if (m_func)
        {
          m_func();
        }
      }
    private:
      std::function<void()> m_func;
    };

    template<typename... T>
    constexpr static bool always_false = false;

    std::filesystem::path get_exe_path();
    void trim(std::string& str, const char* pattern = " \t\n\r\f\v");
    std::string trim(const std::string& str, const char* pattern = " \t\n\r\f\v");
    void rtrim(std::string& str, const char* pattern = " \t\n\r\f\v");
    std::string rtrim(const std::string& str, const char* pattern = " \t\n\r\f\v");
    void ltrim(std::string& str, const char* pattern = " \t\n\r\f\v");
    std::string ltrim(const std::string& str, const char* pattern = " \t\n\r\f\v");
    const std::filesystem::path& get_project_root_dir();
    const std::filesystem::path& get_assets_folder();
    const std::filesystem::path& get_textures_folder();

    void get_meshes_metadata(const std::vector<MeshNode>& mesh_nodes, size_t& nvertices, size_t& nfaces);
    BoundingBox calculate_world_space_bbox(const BoundingBox& local_bbox, const glm::mat4& transform,
                                           glm::mat4* calculated_transform = nullptr);
    BoundingBox calculate_bbox(const std::vector<MeshNode>& mesh_nodes);

    template<typename T, typename U = T>
    void set_flag(T& flag, U bit)
    {
      flag |= bit;
    }

    template<typename T, typename U = T>
    void clear_flag(T& flag, U bit)
    {
      flag &= ~bit;
    }

    template<typename T, typename U = T>
    bool get_flag(T flag, U bit)
    {
      return flag & bit;
    }

    template<typename T, typename U = T>
    void swap_flag(T& flag, U bit)
    {
      if (get_flag(flag, bit))
      {
        clear_flag(flag, bit);
      }
      else
      {
        set_flag(flag, bit);
      }
    }

    template<class T>
    inline void hash_combine(std::size_t& seed, const T& v)
    {
      // https://stackoverflow.com/questions/7222143/unordered-map-hash-function-c
      std::hash<T> hasher;
      seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    constexpr inline uint32_t FNV1a32(std::string_view str)
    {
      // https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
      uint32_t seed = 0x811c9dc5;
      constexpr uint32_t FNV_32_PRIME = 0x01000193;
      for (size_t i = 0; i < str.size(); i++)
      {
        seed ^= static_cast<uint32_t>(str[i]);
        seed *= FNV_32_PRIME;
      }
      return seed;
    }

    template<typename T>
    constexpr std::string_view get_cls_name_helper()
    {
      if constexpr (IsRegisteredClass<T>::value)
      {
        return T::cls_name;
      }
      else
      {
        return typeid(T).name();
      }
    }

    template<typename... Args>
    std::string generate_asset_lookup_name(std::string_view prefix, Args&&... args)
    {
      size_t seed = 0;
      (utils::hash_combine(seed, std::forward<Args>(args)), ...);
      return prefix.data() + std::to_string(seed);
    }

  } // namespace utils
} // namespace fury
