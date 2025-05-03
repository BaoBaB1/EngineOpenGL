#pragma once

#include "./core/Shader.hpp"
#include <map>

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
      LAST_ITEM
    };
    static void init();
    static Shader& get(ShaderType type) { return shaders[type]; }
    static Shader* get(unsigned int id);
  private:
    static std::map<ShaderType, Shader> shaders;
  };
}
