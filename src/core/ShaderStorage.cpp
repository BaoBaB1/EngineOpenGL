#include "ShaderStorage.hpp"
#include "utils/Utils.hpp"

namespace
{
  struct ShaderDescription
  {
    std::vector<std::pair<fury::ShaderStage, std::filesystem::path>> sources;
    fury::ShaderStorage::ShaderType shader_type;
  };
  const std::filesystem::path GLSL_FOLDER = fury::utils::get_project_root_dir() / "src" / "glsl";
}

namespace fury
{
  std::map<ShaderStorage::ShaderType, Shader> ShaderStorage::shaders;

  void ShaderStorage::init()
  {
    if (shaders.empty())
    {
      std::vector<ShaderDescription> descriptions;
      descriptions.reserve(ShaderStorage::LAST_ITEM);
      {
        ShaderDescription d;
        d.sources.push_back({ ShaderStage::VERTEX, GLSL_FOLDER / "default.vert" });
        d.sources.push_back({ ShaderStage::FRAGMENT, GLSL_FOLDER / "default.frag" });
        d.shader_type = ShaderStorage::ShaderType::DEFAULT;
        descriptions.push_back(d);
      }
      {
        ShaderDescription d;
        d.sources.push_back({ ShaderStage::VERTEX,  GLSL_FOLDER / "outlining.vert" });
        d.sources.push_back({ ShaderStage::FRAGMENT, GLSL_FOLDER / "outlining.frag" });
        d.shader_type = ShaderStorage::ShaderType::OUTLINING;
        descriptions.push_back(d);
      }
      {
        ShaderDescription d;
        d.sources.push_back({ ShaderStage::VERTEX, GLSL_FOLDER / "skybox.vert" });
        d.sources.push_back({ ShaderStage::FRAGMENT, GLSL_FOLDER / "skybox.frag" });
        d.shader_type = ShaderStorage::ShaderType::SKYBOX;
        descriptions.push_back(d);
      }
      {
        ShaderDescription d;
        d.sources.push_back({ ShaderStage::VERTEX, GLSL_FOLDER / "screen_quad.vert" });
        d.sources.push_back({ ShaderStage::FRAGMENT, GLSL_FOLDER / "screen_quad.frag" });
        d.shader_type = ShaderStorage::ShaderType::SCREEN_QUAD;
        descriptions.push_back(d);
      }
      {
        ShaderDescription d;
        d.sources.push_back({ ShaderStage::VERTEX, GLSL_FOLDER / "depth_picking.vert" });
        d.sources.push_back({ ShaderStage::FRAGMENT, GLSL_FOLDER / "depth_picking.frag" });
        d.shader_type = ShaderStorage::ShaderType::DEPTH_PICKING;
        descriptions.push_back(d);
      }
      {
        ShaderDescription d;
        d.sources.push_back({ ShaderStage::VERTEX, GLSL_FOLDER / "simple.vert" });
        d.sources.push_back({ ShaderStage::FRAGMENT, GLSL_FOLDER / "simple.frag" });
        d.shader_type = ShaderStorage::ShaderType::SIMPLE;
        descriptions.push_back(d);
      }
      {
        ShaderDescription d;
        d.sources.push_back({ ShaderStage::VERTEX, GLSL_FOLDER / "normals.vert" });
        d.sources.push_back({ ShaderStage::GEOMETRY, GLSL_FOLDER / "normals.geom" });
        d.sources.push_back({ ShaderStage::FRAGMENT, GLSL_FOLDER / "normals.frag" });
        d.shader_type = ShaderStorage::ShaderType::NORMALS;
        descriptions.push_back(d);
      }
      {
        ShaderDescription d;
        d.sources.push_back({ ShaderStage::VERTEX, GLSL_FOLDER / "shadow_map.vert" });
        d.sources.push_back({ ShaderStage::FRAGMENT, GLSL_FOLDER / "shadow_map.frag" });
        d.shader_type = ShaderStorage::ShaderType::SHADOW_MAP;
        descriptions.push_back(d);
      }
      {
        ShaderDescription d;
        d.sources.push_back({ ShaderStage::VERTEX, GLSL_FOLDER / "bounding_box.vert" });
        d.sources.push_back({ ShaderStage::FRAGMENT, GLSL_FOLDER / "simple.frag" });
        d.shader_type = ShaderStorage::ShaderType::BOUNDING_BOX;
        descriptions.push_back(d);
      }
      {
        ShaderDescription d;
        d.sources.push_back({ ShaderStage::VERTEX, GLSL_FOLDER / "selection_wheel.vert" });
        d.sources.push_back({ ShaderStage::FRAGMENT, GLSL_FOLDER / "selection_wheel.frag" });
        d.shader_type = ShaderStorage::ShaderType::SELECTION_WHEEL;
        descriptions.push_back(d);
      }
      {
        ShaderDescription d;
        d.sources.push_back({ ShaderStage::VERTEX, GLSL_FOLDER / "selection_wheel_icon.vert" });
        d.sources.push_back({ ShaderStage::FRAGMENT, GLSL_FOLDER / "selection_wheel_icon.frag" });
        d.shader_type = ShaderStorage::ShaderType::SELECTION_WHEEL_ICON;
        descriptions.push_back(d);
      }
      for (const ShaderDescription& desc : descriptions)
      {
        shaders.emplace(desc.shader_type, Shader(desc.sources));
      }
    }
  }

  Shader* ShaderStorage::get(unsigned int id)
  {
    for (auto& item : shaders)
    {
      if (item.second.id() == id)
        return &(item.second);
    }
    return nullptr;
  }
}
