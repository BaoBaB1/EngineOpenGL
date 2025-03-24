#pragma once

#include "./core/Shader.hpp"
#include <map>

class ShaderStorage
{
public:
  enum ShaderType
  {
    MAIN,
    OUTLINING,
    SKYBOX,
    SCREEN_QUAD,
    DEPTH_PICKING,
    LINES,
    NORMALS,
    LAST_ITEM
  };
  static void init();
  static Shader& get(ShaderType type) { return shaders[type]; }
  static Shader* get(unsigned int id);
private:
  static std::map<ShaderType, Shader> shaders;
};
