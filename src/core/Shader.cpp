#include "Shader.hpp"
#include "Logger.hpp"
#include "opengl/Debug.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <fstream>
#include <exception>
#include <filesystem>

static bool read_shader_file_content(const char* const file, std::string& content) 
{
  if (!std::filesystem::exists(file))
  {
    fury::Logger::error("File {} does not exist", file);
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

namespace fury
{
  Shader::Shader(const ShaderDescription& description)
  {
    init(description);
  }

  void Shader::init(const ShaderDescription& description)
  {
    load(description);
  }

  void Shader::load(const ShaderDescription& description)
  {
    if (description.sources.size() < 2)
    {
      Logger::error("Invalid shader description (size < 2).");
      return;
    }
    if (m_id != 0)
    {
      glDeleteProgram(m_id);
    }
    *id_ref() = glCreateProgram();

    for (const auto& [stage, source] : description.sources)
    {
      std::string content;
      if (!read_shader_file_content(source.string().c_str(), content))
      {
        throw std::runtime_error(fmt::format("Error reading shader file {}.", source.string()));
      }
      GLuint shader = glCreateShader(static_cast<int>(stage));
      auto data = content.data();
      glShaderSource(shader, 1, &data, NULL);
      glCompileShader(shader);
      if (opengl_check_error(fmt::format("glCompileShader failed with source {}", source.string())))
      {
        return;
      }
      glAttachShader(m_id, shader);
      if (opengl_check_error(fmt::format("glAttachShader failed with source  {}", source.string())))
      {
        return;
      }
      glLinkProgram(m_id);
      if (opengl_check_error(fmt::format("glLinkProgram failed with source {}", source.string())))
      {
        return;
      }
      glDeleteShader(shader);
    }
    check_shader(m_id.id, description.name);
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
}
