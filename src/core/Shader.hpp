#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "OpenGLObject.hpp"
#include <vector>
#include <string_view>

struct VertexLayout
{
  constexpr VertexLayout() = default;
  constexpr VertexLayout(uint8_t position_, uint8_t normal_, uint8_t color_, uint8_t uv_)
    : position(position_), normal(normal_), color(color_), uv(uv_) {} 
  uint8_t position = 0;
  uint8_t normal = 1;
  uint8_t color = 2;
  uint8_t uv = 3;
};

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
  OnlyMovable(Shader)
  Shader() = default;
  Shader(const std::vector<std::pair<ShaderStage, std::string_view>>& description, const VertexLayout& layout);
  ~Shader();
  void set_matrix4f(const char* uniform_name, const glm::mat4& value);
  void set_vec3(const char* uniform_name, const glm::vec3& value);
  void set_bool(const char* uniform_name, bool value);
  void set_uint(const char* uniform_name, unsigned int value);
  void set_float(const char* uniform_name, float value);
  void set_int(const char* uniform_name, int value);
  //void set_vertex_layout(const VertexLayout& layout) { m_vertex_layout = layout; }
  void bind() const override;
  void unbind() const override;
  VertexLayout vertex_layout() { return m_vertex_layout; }
  const VertexLayout& vertex_layout() const { return m_vertex_layout; }
private:
  void load(const std::vector<std::pair<ShaderStage, std::string_view>>& description);
private:
  VertexLayout m_vertex_layout;
};
