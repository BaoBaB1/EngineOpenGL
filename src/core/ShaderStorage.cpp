#include "ShaderStorage.hpp"
#include <vector>
#include <utility>
#include <string>
#include <array>

namespace
{
  struct ShaderDescription
  {
    std::vector<std::pair<ShaderStage, std::string_view>> sources;
    VertexLayout vertex_layout;
  };
}

std::map<ShaderStorage::ShaderType, Shader> ShaderStorage::shaders;

void ShaderStorage::init()
{
  static bool once = true;
  if (once)
  {
    std::vector<ShaderDescription> descriptions;
    descriptions.reserve(ShaderStorage::LAST_ITEM + 1);
    {
      ShaderDescription d;
      d.sources.push_back({ ShaderStage::VERTEX, ".//src//glsl//shader.vert" });
      d.sources.push_back({ ShaderStage::FRAGMENT, ".//src//glsl//shader.frag" });
      d.vertex_layout = VertexLayout(0, 1, 2, 3);
      descriptions.push_back(d);
    }
    {
      ShaderDescription d;
      d.sources.push_back({ ShaderStage::VERTEX, ".//src//glsl//outlining.vert" });
      d.sources.push_back({ ShaderStage::FRAGMENT, ".//src//glsl//outlining.frag" });
      d.vertex_layout = VertexLayout(0, 1, -1, -1);
      descriptions.push_back(d);
    }
    {
      ShaderDescription d;
      d.sources.push_back({ ShaderStage::VERTEX, ".//src//glsl//skybox.vert" });
      d.sources.push_back({ ShaderStage::FRAGMENT, ".//src//glsl//skybox.frag" });
      d.vertex_layout = VertexLayout(0, -1, -1, -1);
      descriptions.push_back(d);
    }
    {
      ShaderDescription d;
      d.sources.push_back({ ShaderStage::VERTEX, ".//src//glsl//screen_quad.vert" });
      d.sources.push_back({ ShaderStage::FRAGMENT, ".//src//glsl//screen_quad.frag" });
      d.vertex_layout = VertexLayout(0, -1, -1, 1);
      descriptions.push_back(d);
    }
    {
      ShaderDescription d;
      d.sources.push_back({ ShaderStage::VERTEX, ".//src//glsl//depth_picking.vert" });
      d.sources.push_back({ ShaderStage::FRAGMENT, ".//src//glsl//depth_picking.frag" });
      d.vertex_layout = VertexLayout(0, -1, -1, -1);
      descriptions.push_back(d);
    }
    {
      ShaderDescription d;
      d.sources.push_back({ ShaderStage::VERTEX, ".//src//glsl//lines.vert" });
      d.sources.push_back({ ShaderStage::FRAGMENT, ".//src//glsl//lines.frag" });
      d.vertex_layout = VertexLayout(0, -1, 1, -1);
      descriptions.push_back(d);
    }
    {
      ShaderDescription d;
      d.sources.push_back({ ShaderStage::VERTEX, ".//src//glsl//normals.vert" });
      d.sources.push_back({ ShaderStage::GEOMETRY, ".//src//glsl//normals.geom" });
      d.sources.push_back({ ShaderStage::FRAGMENT, ".//src//glsl//normals.frag" });
      d.vertex_layout = VertexLayout(0, 1, -1, -1);
      descriptions.push_back(d);
    }
    for (int i = 0; i < ShaderStorage::LAST_ITEM; i++)
    {
      shaders.emplace(static_cast<ShaderStorage::ShaderType>(i), Shader(descriptions[i].sources, descriptions[i].vertex_layout));
    }
    once = false;
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
