#pragma once

#include "Logger.hpp"
#include <glad/glad.h>
#include <string>

namespace fury
{
  inline int opengl_check_error(const std::string& msg)
  {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
      Logger::error("OpenGL error {}. {}.", error, msg);
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
      Logger::error("{} failed : {}", param_name, std::string(infoLog, len));
    }
  }
}
