#include "Utils.hpp"
#include "core/Logger.hpp"

namespace
{
  std::filesystem::path find_root_dir()
  {
    auto path = fury::utils::get_exe_path();
    bool found = false;
    while (path.has_relative_path())
    {
      path = path.parent_path();
      // find top-most assets folder within exe path
      if (std::filesystem::exists(path / "assets"))
      {
        found = true;
        break;
      }
    }
    if (!found)
    {
      fury::Logger::critical("Could not find project root directory from exe path {}.", fury::utils::get_exe_path().string());
      throw std::runtime_error("Could not find project root directory");
    }
    return path;
  }
}

namespace fury
{
  namespace utils
  {
    const std::filesystem::path& get_project_root_dir()
    {
      static std::filesystem::path root_path = find_root_dir();
      return root_path;
    }

    void trim(std::string& str, const char* pattern)
    {
      ltrim(str);
      rtrim(str);
    }

    std::string trim(const std::string& str, const char* pattern)
    {
      return rtrim(ltrim(str));
    }

    void rtrim(std::string& str, const char* pattern)
    {
      size_t idx = str.find_last_not_of(pattern);
      if (idx != std::string::npos)
      {
        str.erase(idx + 1);
      }
    }

    std::string rtrim(const std::string& str, const char* pattern)
    {
      size_t idx = str.find_last_not_of(pattern);
      if (idx != std::string::npos)
      {
        return str.substr(0, idx + 1);
      }
      return str;
    }

    void ltrim(std::string& str, const char* pattern)
    {
      size_t idx = str.find_first_not_of(pattern);
      if (idx != std::string::npos)
      {
        str.erase(0, idx);
      }
    }

    std::string ltrim(const std::string& str, const char* pattern)
    {
      size_t idx = str.find_first_not_of(pattern);
      if (idx != std::string::npos)
      {
        return str.substr(idx);
      }
      return str;
    }
  } // utils namespace
} // fury namespace
