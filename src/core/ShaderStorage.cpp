#include "ShaderStorage.hpp"
#include "utils/Utils.hpp"
#include <ranges>
#include <span>

namespace fury
{
  static const std::filesystem::path GLSL_FOLDER = fury::utils::get_project_root_dir() / "src" / "glsl";

  static void generate_defines_combinations_backtrack(size_t start_idx, std::vector<std::string>& current,
                                                      std::vector<std::vector<std::string>>& result,
                                                      const std::vector<std::string>& defines)
  {
    if (!current.empty())
    {
      result.push_back(current);
    }

    for (size_t i = start_idx; i < defines.size(); i++)
    {
      current.push_back(defines[i]);
      generate_defines_combinations_backtrack(i + 1, current, result, defines);
      current.pop_back();
    }
  }

  static std::vector<std::vector<std::string>> generate_defines_combinations(const std::vector<std::string>& defines)
  {
    std::vector<std::vector<std::string>> result;
    std::vector<std::string> current;
    generate_defines_combinations_backtrack(0, current, result, defines);
    return result;
  }

  void ShaderStorage::create_shader(const ShaderDescription& desc, ShaderType type, bool do_defines_combinations)
  {
    static auto create = [](const ShaderDescription& desc, ShaderType type)
    {
      ShaderEntry& entry = shader_entries.emplace_back();
      entry.defines = desc.defines;
      entry.shader_type = type;
      entry.shader.init(desc);
    };

    if (!do_defines_combinations || desc.defines.empty())
    {
      create(desc, type);
    }
    else
    {
      ShaderDescription desc2 = desc;
      auto defines_combinations = generate_defines_combinations(desc.defines);
      for (const auto& c : defines_combinations)
      {
        desc2.defines = c;
        create(desc2, type);
      }
      desc2.defines.clear();
      // Add shader with 0 defines
      create(desc2, type);
    }
  }

  void ShaderStorage::init()
  {
    shader_entries.clear();
    shader_entries.reserve(ShaderStorage::LAST_ITEM);

    create_shader(ShaderDescription { .sources = { { ShaderStage::VERTEX, GLSL_FOLDER / "default.vert" },
                                                   { ShaderStage::FRAGMENT, GLSL_FOLDER / "default.frag" } },
                                      .name = "Default",
                                      .defines = { "USE_VERTEX_COLOR", "APPLY_LIGHTING", "HAS_DIFFUSE_TEXTURE",
                                                   "HAS_AMBIENT_TEXTURE", "HAS_SPECULAR_TEXTURE" } },
                  ShaderType::DEFAULT, true);

    create_shader(ShaderDescription { .sources = { { ShaderStage::VERTEX, GLSL_FOLDER / "outlining.vert" },
                                                   { ShaderStage::FRAGMENT, GLSL_FOLDER / "outlining.frag" } },
                                      .name = "Outlining" },
                  ShaderType::OUTLINING, false);

    create_shader(ShaderDescription { .sources = { { ShaderStage::VERTEX, GLSL_FOLDER / "skybox.vert" },
                                                   { ShaderStage::FRAGMENT, GLSL_FOLDER / "skybox.frag" } },
                                      .name = "Skybox" },
                  ShaderType::SKYBOX, false);

    create_shader(ShaderDescription { .sources = { { ShaderStage::VERTEX, GLSL_FOLDER / "screen_quad.vert" },
                                                   { ShaderStage::FRAGMENT, GLSL_FOLDER / "screen_quad.frag" } },
                                      .name = "Screen quad" },
                  ShaderType::SCREEN_QUAD, false);

    create_shader(ShaderDescription { .sources = { { ShaderStage::VERTEX, GLSL_FOLDER / "depth_picking.vert" },
                                                   { ShaderStage::FRAGMENT, GLSL_FOLDER / "depth_picking.frag" } },
                                      .name = "Depth picking" },
                  ShaderType::DEPTH_PICKING, false);

    create_shader(ShaderDescription { .sources = { { ShaderStage::VERTEX, GLSL_FOLDER / "simple.vert" },
                                                   { ShaderStage::FRAGMENT, GLSL_FOLDER / "simple.frag" } },
                                      .name = "Simple" },
                  ShaderType::SIMPLE, false);

    create_shader(ShaderDescription { .sources = { { ShaderStage::VERTEX, GLSL_FOLDER / "normals.vert" },
                                                   { ShaderStage::GEOMETRY, GLSL_FOLDER / "normals.geom" },
                                                   { ShaderStage::FRAGMENT, GLSL_FOLDER / "normals.frag" } },
                                      .name = "Normals" },
                  ShaderType::NORMALS, false);

    create_shader(ShaderDescription { .sources = { { ShaderStage::VERTEX, GLSL_FOLDER / "shadow_map.vert" },
                                                   { ShaderStage::FRAGMENT, GLSL_FOLDER / "shadow_map.frag" } },
                                      .name = "Shadow map" },
                  ShaderType::SHADOW_MAP, false);

    create_shader(ShaderDescription { .sources = { { ShaderStage::VERTEX, GLSL_FOLDER / "bounding_box.vert" },
                                                   { ShaderStage::FRAGMENT, GLSL_FOLDER / "simple.frag" } },
                                      .name = "Bounding box" },
                  ShaderType::BOUNDING_BOX, false);

    create_shader(ShaderDescription { .sources = { { ShaderStage::VERTEX, GLSL_FOLDER / "selection_wheel.vert" },
                                                   { ShaderStage::FRAGMENT, GLSL_FOLDER / "selection_wheel.frag" } },
                                      .name = "Selection wheel" },
                  ShaderType::SELECTION_WHEEL, false);

    create_shader(
        ShaderDescription { .sources = { { ShaderStage::VERTEX, GLSL_FOLDER / "selection_wheel_icon.vert" },
                                         { ShaderStage::FRAGMENT, GLSL_FOLDER / "selection_wheel_icon.frag" } },
                            .name = "Selection wheel icon" },
        ShaderType::SELECTION_WHEEL_ICON, false);

    create_shader(ShaderDescription { .sources = { { ShaderStage::VERTEX, GLSL_FOLDER / "grid.vert" },
                                                   { ShaderStage::FRAGMENT, GLSL_FOLDER / "grid.frag" } },
                                      .name = "Grid" },
                  ShaderType::GRID, false);

    create_shader(ShaderDescription { .sources = { { ShaderStage::VERTEX, GLSL_FOLDER / "simple_with_vcolor.vert" },
                                                   { ShaderStage::FRAGMENT, GLSL_FOLDER / "simple_with_vcolor.frag" } },
                                      .name = "Simple with color" },
                  ShaderType::SIMPLE_WITH_VCOLOR, false);
  }

  Shader* ShaderStorage::get(ShaderType type, std::span<std::string> defines)
  {
    for (ShaderEntry& entry : shader_entries)
    {
      if (entry.shader_type == type && std::ranges::is_permutation(entry.defines, defines))
      {
        return &entry.shader;
      }
    }
    assert(false && "Never here");
    return nullptr;
  }

} // namespace fury
