#include "Utils.hpp"
#include "core/Logger.hpp"
#include "core/ecs/components/DrawableComponent.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <limits>

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
      fury::Logger::critical("Could not find project root directory from exe path {}.",
                             fury::utils::get_exe_path().string());
      throw std::runtime_error("Could not find project root directory");
    }
    return path;
  }
} // namespace

namespace fury
{
  namespace utils
  {
    const std::filesystem::path& get_project_root_dir()
    {
      static std::filesystem::path root_path = find_root_dir();
      return root_path;
    }

    const std::filesystem::path& get_assets_folder()
    {
      const static std::filesystem::path assets_folder = utils::get_project_root_dir() / "assets";
      return assets_folder;
    }

    const std::filesystem::path& get_textures_folder()
    {
      const static std::filesystem::path textures_folder = utils::get_assets_folder() / "textures";
      return textures_folder;
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
      {
        return std::filesystem::path { szPath }.parent_path() / ""; // to finish the folder path with (back)slash
      }
      return {}; // some error
#else
      // Linux specific
      char szPath[PATH_MAX];
      ssize_t count = readlink("/proc/self/exe", szPath, PATH_MAX);
      if (count < 0 || count >= PATH_MAX)
      {
        return {}; // some error
      }
      szPath[count] = '\0';
      return std::filesystem::path(szPath);
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

    BoundingBox calculate_bbox(const std::vector<MeshNode>& mesh_nodes)
    {
      glm::vec3 min(std::numeric_limits<float>::max()), max(std::numeric_limits<float>::min());
      for (const MeshNode& mesh_node : mesh_nodes)
      {
        if (!mesh_node.mesh.asset->get_bbox().is_valid())
        {
          mesh_node.mesh.asset->calculate_bbox();
        }
        min = glm::min(min, mesh_node.mesh.asset->get_bbox().min());
        max = glm::max(max, mesh_node.mesh.asset->get_bbox().max());
      }
      return BoundingBox(min, max);
    }

    BoundingBox calculate_world_space_bbox(const BoundingBox& local_bbox, const glm::mat4& transform,
                                           glm::mat4* calculated_transform)
    {
      // Jim Arvo's variation of fast bbox recompute
      const glm::vec3& min = local_bbox.min();
      const glm::vec3& max = local_bbox.max();
      const glm::vec3 center = local_bbox.center();
      const glm::vec3 half_extents = (max - min) * 0.5f;
      const glm::mat3 mm = { glm::abs(transform[0]), glm::abs(transform[1]), glm::abs(transform[2]) };
      const glm::vec3 center_world = transform * glm::vec4(center, 1);
      const glm::vec3 extents_world = (mm * half_extents) * 2.f;
      if (calculated_transform)
      {
        glm::mat4 out_mat(1.f);
        out_mat = glm::translate(out_mat, center_world) * glm::scale(out_mat, extents_world);
        //  OBB
        //  out_mat = transform * (glm::translate(out_mat, center) * glm::scale(out_mat, extents));
        *calculated_transform = out_mat;
      }
      return BoundingBox(center_world - (extents_world * 0.5f), center_world + (extents_world * 0.5f));
    }

    void get_meshes_metadata(const std::vector<MeshNode>& mesh_nodes, size_t& nvertices, size_t& nfaces)
    {
      nvertices = nfaces = 0;
      for (const auto& node : mesh_nodes)
      {
        if (node.mesh) [[likely]]
        {
          nvertices += node.mesh.asset->get_vertices().size();
          nfaces += node.mesh.asset->get_faces().size();
        }
      }
    }

  } // namespace utils
} // namespace fury
