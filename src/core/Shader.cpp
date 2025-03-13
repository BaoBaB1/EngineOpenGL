#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Shader.hpp"
#include <string>
#include <fstream>
#include <exception>
#include <filesystem>

static bool read_shader_file_content(const char* const file, std::string& content) 
{
  if (!std::filesystem::exists(file))
  {
    std::cerr << "File " << file << " does not exist\n";
    return false;
  }
  content.clear();
  std::ifstream in;
  in.open(file, std::ios_base::binary);
  if (in.is_open()) {
    in.seekg(0, std::ios::end);
    size_t sz = in.tellg();
    content.resize(sz);
    in.seekg(0, std::ios::beg);
    in.read(&content[0], sz);
    return true;
  }
  return false;
}

Shader::Shader(const std::vector<std::pair<ShaderStage, std::string_view>>& description, const VertexLayout& layout)
{
  assert(description.size() >= 2);
  load(description);
  m_vertex_layout = layout;
}

void Shader::load(const std::vector<std::pair<ShaderStage, std::string_view>>& description)
{
  if (description.size() < 2)
  {
    std::cerr << "Invalid shader description (size < 2).\n";
    return;
  }
  if (m_id != 0)
  {
    glDeleteProgram(m_id);
  }
  *id_ref() = glCreateProgram();

  for (const auto& [stage, source] : description)
  {
    std::string content;
    if (!read_shader_file_content(source.data(), content))
    {
      throw std::runtime_error(std::string("Error reading shader file ") + source.data() + "\n");
    }
    GLuint shader = glCreateShader(static_cast<int>(stage));
    auto data = content.data();
    glShaderSource(shader, 1, &data, NULL);
    glCompileShader(shader);
    if (::opengl_check_error(std::string("glCompileShader failed with source ") + source.data()))
    {
      return;
    }
    glAttachShader(m_id, shader);
    if (::opengl_check_error(std::string("glAttachShader failed with source ") + source.data()))
    {
      return;
    }
    glLinkProgram(m_id);
    if (::opengl_check_error(std::string("glLinkProgram failed with source ") + source.data()))
    {
      return;
    }
    glDeleteShader(shader);
  }
  ::opengl_check_shader_program(m_id.id, GL_VALIDATE_STATUS, "GL_VALIDATE_STATUS");
  ::opengl_check_shader_program(m_id.id, GL_LINK_STATUS, "GL_LINK_STATUS");
}

void Shader::set_matrix4f(const char* uniform_name, const glm::mat4& value) 
{
  glUniformMatrix4fv(glGetUniformLocation(m_id, uniform_name), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::set_vec3(const char* uniform_name, const glm::vec3& value) 
{
  glUniform3fv(glGetUniformLocation(m_id, uniform_name), 1, glm::value_ptr(value));
}

void Shader::set_bool(const char* uniform_name, bool value) 
{
  set_int(uniform_name, value);
}

void Shader::set_int(const char* uniform_name, int value)
{
  glUniform1i(glGetUniformLocation(m_id, uniform_name), value);
}

void Shader::set_uint(const char* uniform_name, unsigned int value)
{
  glUniform1ui(glGetUniformLocation(m_id, uniform_name), value);
}

void Shader::set_float(const char* uniform_name, float value) 
{
  glUniform1f(glGetUniformLocation(m_id, uniform_name), value);
}

void Shader::bind() const 
{
  glUseProgram(m_id);
}

void Shader::unbind() const
{
  glUseProgram(0);
}

Shader::~Shader() 
{
  glDeleteProgram(m_id);
}
