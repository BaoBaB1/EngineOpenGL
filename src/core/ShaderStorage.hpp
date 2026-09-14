#pragma once

#include "Shader.hpp"
#include <vector>
#include <span>

namespace fury
{
  class ShaderStorage
  {
  public:
    enum ShaderType
    {
      DEFAULT,
      OUTLINING,
      SKYBOX,
      SCREEN_QUAD,
      DEPTH_PICKING,
      SIMPLE,
      NORMALS,
      SHADOW_MAP,
      BOUNDING_BOX,
      SELECTION_WHEEL,
      SELECTION_WHEEL_ICON,
      GRID,
      SIMPLE_WITH_VCOLOR,
      LAST_ITEM
    };
    static void init();
    static void create_shader(const ShaderDescription& desc, ShaderType type, bool do_defines_permutations);
    static Shader* get(ShaderType type, std::span<std::string> defines = {});
  private:
    struct ShaderEntry
    {
      // ShaderDescription desc;
      Shader shader;
      std::vector<std::string> defines;
      fury::ShaderStorage::ShaderType shader_type;
    };
  private:
    inline static std::vector<ShaderEntry> shader_entries;
  };
} // namespace fury
