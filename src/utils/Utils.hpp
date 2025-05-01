#pragma once

#include <filesystem>
#include <string>
#include <vector>

#ifdef _WIN32
  #include <Windows.h>
#elif __APPLE__
  #include <mach-o/dyld.h>
  #include <climits>
#else
  #include <unistd.h>
#endif


namespace fury
{
  namespace utils
  {
    inline std::filesystem::path get_exe_path()
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
    void trim(std::string& str, const char* pattern = " \t\n\r\f\v");
    std::string trim(const std::string& str, const char* pattern = " \t\n\r\f\v");
    void rtrim(std::string& str, const char* pattern = " \t\n\r\f\v");
    std::string rtrim(const std::string& str, const char* pattern = " \t\n\r\f\v");
    void ltrim(std::string& str, const char* pattern = " \t\n\r\f\v");
    std::string ltrim(const std::string& str, const char* pattern = " \t\n\r\f\v");
    const std::filesystem::path& get_project_root_dir();
  } // utils namespace
} // fury namespace
