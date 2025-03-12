#include "ShaderStorage.hpp"
#include <vector>
#include <utility>
#include <string>
#include <array>

namespace
{
  struct ShaderDescription
  {
    constexpr ShaderDescription(std::string_view vs, std::string_view fs, const VertexLayout& layout)
      : vertex_shader(vs), fragment_shader(fs), vertex_layout(layout) {}
    constexpr ShaderDescription() = default;
    constexpr ShaderDescription(const ShaderDescription&) = default;
    constexpr ShaderDescription(ShaderDescription&&) noexcept = default;
    constexpr ShaderDescription& operator=(const ShaderDescription&) = default;
    constexpr ShaderDescription& operator=(ShaderDescription&&) noexcept = default;
    std::string_view vertex_shader;
    std::string_view fragment_shader;
    VertexLayout vertex_layout;
  };
}

namespace GlobalState
{
  std::map<ShaderStorage::ShaderType, Shader> ShaderStorage::shaders;

  void ShaderStorage::init()
  {
    static bool once = true;
    if (once)
    {
      constexpr std::array descriptions =
      {
        ShaderDescription(".//src//glsl//shader.vert", ".//src//glsl//shader.frag", VertexLayout(0, 1, 2, 3)),
        ShaderDescription(".//src//glsl//outlining.vert", ".//src//glsl//outlining.frag", VertexLayout(0, 1, -1, -1)),
        ShaderDescription(".//src//glsl//skybox.vert", ".//src//glsl//skybox.frag", VertexLayout(0, -1, -1, -1)),
        ShaderDescription(".//src//glsl//fbo_default_shader.vert", ".//src//glsl//fbo_default_shader.frag", VertexLayout(0, -1, -1, 1)),
        ShaderDescription(".//src//glsl//picking_fbo.vert", ".//src//glsl//picking_fbo.frag", VertexLayout(0, -1, -1, -1)),
        ShaderDescription(".//src//glsl//lines.vert", ".//src//glsl//lines.frag", VertexLayout(0, -1, 1, -1))
      };

      for (int i = 0; i < ShaderStorage::LAST_ITEM; i++)
      {
        shaders.emplace(static_cast<ShaderStorage::ShaderType>(i), 
          Shader(descriptions[i].vertex_shader.data(), descriptions[i].fragment_shader.data(), descriptions[i].vertex_layout));
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
};
