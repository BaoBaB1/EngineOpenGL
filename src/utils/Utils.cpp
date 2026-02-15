#include "Utils.hpp"
#include "core/Logger.hpp"

#ifdef _WIN32
  #include <Windows.h>
#elif __APPLE__
  #include <mach-o/dyld.h>
  #include <climits>
#else
  #include <unistd.h>
#endif

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

    std::filesystem::path get_exe_path()
    {
      // https://stackoverflow.com/questions/50889647/best-way-to-get-exe-folder-path/51023983#51023983
      // not tested for anything but windows
#ifdef _WIN32
      wchar_t szPath[MAX_PATH];
      GetModuleFileNameW(NULL, szPath, MAX_PATH);
      return std::filesystem::path(szPath);
#elif __APPLE__
      char szPath[PATH_MAX];
      uint32_t bufsize = PATH_MAX;
      if (!_NSGetExecutablePath(szPath, &bufsize))
        return std::filesystem::path{ szPath }.parent_path() / ""; // to finish the folder path with (back)slash
      return {};  // some error
#else
      // Linux specific
      char szPath[PATH_MAX];
      ssize_t count = readlink("/proc/self/exe", szPath, PATH_MAX);
      if (count < 0 || count >= PATH_MAX)
        return {}; // some error
      szPath[count] = '\0';
      return std::filesystem::path(szPath);
    }
#endif
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
