#pragma once

#include "opengl/OpenGLObject.hpp"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string_view>
#include <filesystem>

namespace fury
{
  enum class ShaderStage
  {
    VERTEX = GL_VERTEX_SHADER,
    FRAGMENT = GL_FRAGMENT_SHADER,
    GEOMETRY = GL_GEOMETRY_SHADER,
    UNKNOWN
  };

  class Shader : public OpenGLObject
  {
  public:
    FURY_OnlyMovable(Shader)
    Shader() = default;
    Shader(const std::vector<std::pair<ShaderStage, std::filesystem::path>>& description);
    ~Shader();
    void set_matrix4f(const char* uniform_name, const glm::mat4& value);
    void set_vec3(const char* uniform_name, const glm::vec3& value);
    void set_bool(const char* uniform_name, bool value);
    void set_uint(const char* uniform_name, unsigned int value);
    void set_float(const char* uniform_name, float value);
    void set_int(const char* uniform_name, int value);
    void bind() const override;
    void unbind() const override;
  private:
    void load(const std::vector<std::pair<ShaderStage, std::filesystem::path>>& description);
  };
}
