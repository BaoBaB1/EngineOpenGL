#pragma once

#include <glad/glad.h>
#include <iostream>
#include <string>

#define DEBUG(x) do { std::cerr << x; } while (0)

inline int opengl_check_error(const std::string& msg)
{
  GLenum error = glGetError();
  if (error != GL_NO_ERROR)
  {
    std::cerr << "OpenGL error " << error << ".\n" << msg << std::endl;
  }
  return error;
}

inline void opengl_check_shader_program(GLuint id, int param_namei, const std::string& param_name)
{
  int status;
  glGetProgramiv(id, param_namei, &status);
  if (status != GL_TRUE)
  {
    int32_t len = 0;
    GLchar infoLog[512];
    glGetShaderInfoLog(id, 512, &len, infoLog);
    std::cerr << param_name + " failed: " << std::string(infoLog, len) << '\n';
  }
}
