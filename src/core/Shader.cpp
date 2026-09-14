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
#include <algorithm>

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
  if (in.is_open())
  {
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
      std::string src = "#version 460 core\n", content;
      if (!description.defines.empty())
      {
        std::for_each(description.defines.begin(), description.defines.end(),
                      [&](const std::string& def) { src += "#define " + def + "\n"; });
      }

      if (!read_shader_file_content(source.string().c_str(), content))
      {
        throw std::runtime_error(fmt::format("Error reading shader file {}.", source.string()));
      }
      src += std::move(content);

      GLuint shader = glCreateShader(static_cast<int>(stage));
      if (shader == 0)
      {
        Logger::error("Failed to create shader stage {} from source {}", static_cast<int>(stage), source);
        continue;
      }
      utils::ScopeGuard scope_guard([shader]() { glDeleteShader(shader); });
      auto data = src.data();
      glShaderSource(shader, 1, &data, NULL);
      GLenum error = opengl_check_error();
      if (error != GL_NO_ERROR)
      {
        Logger::error("glShaderSource failed with source {}", source);
        continue;
      }
      glCompileShader(shader);
      error = opengl_check_error();
      if (error != GL_NO_ERROR)
      {
        Logger::error("glCompileShader failed with source {}", source);
        continue;
      }
      glAttachShader(m_id, shader);
      error = opengl_check_error();
      if (error != GL_NO_ERROR)
      {
        Logger::error("glAttachShader failed with source {}", source);
        continue;
      }
      glLinkProgram(m_id);
      error = opengl_check_error();
      if (error != GL_NO_ERROR)
      {
        Logger::error("glLinkProgram failed with source {}", source);
        continue;
      }
    }
    opengl_check_shader(m_id.id, description);
  }

  void Shader::set_matrix4f(const char* uniform_name, const glm::mat4& value)
  {
    glUniformMatrix4fv(glGetUniformLocation(m_id, uniform_name), 1, GL_FALSE, glm::value_ptr(value));
  }

  void Shader::set_vec3(const char* uniform_name, const glm::vec3& value)
  {
    glUniform3fv(glGetUniformLocation(m_id, uniform_name), 1, glm::value_ptr(value));
  }

  void Shader::set_vec4(const char* uniform_name, const glm::vec4& value)
  {
    glUniform4fv(glGetUniformLocation(m_id, uniform_name), 1, glm::value_ptr(value));
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
} // namespace fury
