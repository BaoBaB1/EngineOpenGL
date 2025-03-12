#pragma once

#include <map>
#include "./core/Shader.hpp"

namespace GlobalState
{
  class ShaderStorage
  {
  public:
    enum ShaderType
    {
      MAIN,
      OUTLINING,
      SKYBOX,
      FBO_DEFAULT,
      PICKING,
      LINES,
      LAST_ITEM
    };
    static void init();
    static Shader& get(ShaderType type) { return shaders[type]; }
    static Shader* get(unsigned int id);
  private:
    static std::map<ShaderType, Shader> shaders;
  };
}
