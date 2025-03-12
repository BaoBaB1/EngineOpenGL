#include <string>
#include <fstream>
#include <exception>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Shader.hpp"

static bool read_shader_file_content(const char* const file, std::string& content) 
{
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

Shader::Shader(const char* vertex_file, const char* fragment_file, const VertexLayout& layout)
{
  load(vertex_file, fragment_file);
  m_vertex_layout = layout;
}

void Shader::load(const char* vertex_file, const char* fragment_file)
{
  if (m_id != 0)
  {
    glDeleteProgram(m_id);
  }
  std::string vertex_shader_source, fragment_shader_source;
  if (!read_shader_file_content(vertex_file, vertex_shader_source))
    throw std::runtime_error("Error reading vertex shader file");
  if (!read_shader_file_content(fragment_file, fragment_shader_source))
    throw std::runtime_error("Error reading fragment shader file");

  const char* vss = vertex_shader_source.c_str();
  const char* fss = fragment_shader_source.c_str();

  // Create Vertex Shader Object and get its reference
  GLuint m_vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  // Attach Vertex Shader source to the Vertex Shader Object
  glShaderSource(m_vertex_shader, 1, &vss, NULL);
  glCompileShader(m_vertex_shader);

  // Create Fragment Shader Object and get its reference
  GLuint m_fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  // Attach Fragment Shader source to the Fragment Shader Object
  glShaderSource(m_fragment_shader, 1, &fss, NULL);
  glCompileShader(m_fragment_shader);

  // Create Shader Program Object and get its reference
  *id_ref() = glCreateProgram();
  glAttachShader(m_id, m_vertex_shader);
  glAttachShader(m_id, m_fragment_shader);
  // Wrap-up/Link all the shaders together into the Shader Program
  glLinkProgram(m_id);

  // Delete the now useless Vertex and Fragment Shader objects
  glDeleteShader(m_vertex_shader);
  glDeleteShader(m_fragment_shader);
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
