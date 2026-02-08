#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <string_view>

namespace fury
{
  namespace utils
  {
    std::filesystem::path get_exe_path();
    void trim(std::string& str, const char* pattern = " \t\n\r\f\v");
    std::string trim(const std::string& str, const char* pattern = " \t\n\r\f\v");
    void rtrim(std::string& str, const char* pattern = " \t\n\r\f\v");
    std::string rtrim(const std::string& str, const char* pattern = " \t\n\r\f\v");
    void ltrim(std::string& str, const char* pattern = " \t\n\r\f\v");
    std::string ltrim(const std::string& str, const char* pattern = " \t\n\r\f\v");
    const std::filesystem::path& get_project_root_dir();

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

  } // utils namespace
} // fury namespace
