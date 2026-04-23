#include "ShaderStorage.hpp"
#include "utils/Utils.hpp"

namespace
{
  struct ShaderDescriptionInternal : fury::ShaderDescription
  {
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
      std::vector<ShaderDescriptionInternal> descriptions;
      descriptions.reserve(ShaderStorage::LAST_ITEM);
      {
        ShaderDescriptionInternal d;
        d.sources.push_back({ ShaderStage::VERTEX, GLSL_FOLDER / "default.vert" });
        d.sources.push_back({ ShaderStage::FRAGMENT, GLSL_FOLDER / "default.frag" });
        d.shader_type = ShaderStorage::ShaderType::DEFAULT;
        d.name = "Default";
        descriptions.push_back(d);
      }
      {
        ShaderDescriptionInternal d;
        d.sources.push_back({ ShaderStage::VERTEX,  GLSL_FOLDER / "outlining.vert" });
        d.sources.push_back({ ShaderStage::FRAGMENT, GLSL_FOLDER / "outlining.frag" });
        d.shader_type = ShaderStorage::ShaderType::OUTLINING;
        d.name = "Outlining";
        descriptions.push_back(d);
      }
      {
        ShaderDescriptionInternal d;
        d.sources.push_back({ ShaderStage::VERTEX, GLSL_FOLDER / "skybox.vert" });
        d.sources.push_back({ ShaderStage::FRAGMENT, GLSL_FOLDER / "skybox.frag" });
        d.shader_type = ShaderStorage::ShaderType::SKYBOX;
        d.name = "Skybox";
        descriptions.push_back(d);
      }
      {
        ShaderDescriptionInternal d;
        d.sources.push_back({ ShaderStage::VERTEX, GLSL_FOLDER / "screen_quad.vert" });
        d.sources.push_back({ ShaderStage::FRAGMENT, GLSL_FOLDER / "screen_quad.frag" });
        d.shader_type = ShaderStorage::ShaderType::SCREEN_QUAD;
        d.name = "Screen quad";
        descriptions.push_back(d);
      }
      {
        ShaderDescriptionInternal d;
        d.sources.push_back({ ShaderStage::VERTEX, GLSL_FOLDER / "depth_picking.vert" });
        d.sources.push_back({ ShaderStage::FRAGMENT, GLSL_FOLDER / "depth_picking.frag" });
        d.shader_type = ShaderStorage::ShaderType::DEPTH_PICKING;
        d.name = "Depth picking";
        descriptions.push_back(d);
      }
      {
        ShaderDescriptionInternal d;
        d.sources.push_back({ ShaderStage::VERTEX, GLSL_FOLDER / "simple.vert" });
        d.sources.push_back({ ShaderStage::FRAGMENT, GLSL_FOLDER / "simple.frag" });
        d.shader_type = ShaderStorage::ShaderType::SIMPLE;
        d.name = "Simple";
        descriptions.push_back(d);
      }
      {
        ShaderDescriptionInternal d;
        d.sources.push_back({ ShaderStage::VERTEX, GLSL_FOLDER / "normals.vert" });
        d.sources.push_back({ ShaderStage::GEOMETRY, GLSL_FOLDER / "normals.geom" });
        d.sources.push_back({ ShaderStage::FRAGMENT, GLSL_FOLDER / "normals.frag" });
        d.shader_type = ShaderStorage::ShaderType::NORMALS;
        d.name = "Normals";
        descriptions.push_back(d);
      }
      {
        ShaderDescriptionInternal d;
        d.sources.push_back({ ShaderStage::VERTEX, GLSL_FOLDER / "shadow_map.vert" });
        d.sources.push_back({ ShaderStage::FRAGMENT, GLSL_FOLDER / "shadow_map.frag" });
        d.shader_type = ShaderStorage::ShaderType::SHADOW_MAP;
        d.name = "Shadow map";
        descriptions.push_back(d);
      }
      {
        ShaderDescriptionInternal d;
        d.sources.push_back({ ShaderStage::VERTEX, GLSL_FOLDER / "bounding_box.vert" });
        d.sources.push_back({ ShaderStage::FRAGMENT, GLSL_FOLDER / "simple.frag" });
        d.shader_type = ShaderStorage::ShaderType::BOUNDING_BOX;
        d.name = "Bounding box";
        descriptions.push_back(d);
      }
      {
        ShaderDescriptionInternal d;
        d.sources.push_back({ ShaderStage::VERTEX, GLSL_FOLDER / "selection_wheel.vert" });
        d.sources.push_back({ ShaderStage::FRAGMENT, GLSL_FOLDER / "selection_wheel.frag" });
        d.shader_type = ShaderStorage::ShaderType::SELECTION_WHEEL;
        d.name = "Selection wheel";
        descriptions.push_back(d);
      }
      {
        ShaderDescriptionInternal d;
        d.sources.push_back({ ShaderStage::VERTEX, GLSL_FOLDER / "selection_wheel_icon.vert" });
        d.sources.push_back({ ShaderStage::FRAGMENT, GLSL_FOLDER / "selection_wheel_icon.frag" });
        d.shader_type = ShaderStorage::ShaderType::SELECTION_WHEEL_ICON;
        d.name = "Selection wheel icon";
        descriptions.push_back(d);
      }
      {
        ShaderDescriptionInternal d;
        d.sources.push_back({ ShaderStage::VERTEX, GLSL_FOLDER / "grid.vert" });
        d.sources.push_back({ ShaderStage::FRAGMENT, GLSL_FOLDER / "grid.frag" });
        d.shader_type = ShaderStorage::ShaderType::GRID;
        d.name = "Infinite grid";
        descriptions.push_back(d);
      }
      {
        ShaderDescriptionInternal d;
        d.sources.push_back({ ShaderStage::VERTEX, GLSL_FOLDER / "simple_with_vcolor.vert" });
        d.sources.push_back({ ShaderStage::FRAGMENT, GLSL_FOLDER / "simple_with_vcolor.frag" });
        d.shader_type = ShaderStorage::ShaderType::SIMPLE_WITH_VCOLOR;
        d.name = "Simple with color";
        descriptions.push_back(d);
      }
      for (const ShaderDescriptionInternal& desc : descriptions)
      {
        shaders.emplace(desc.shader_type, Shader(desc));
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
