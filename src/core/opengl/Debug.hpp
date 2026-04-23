#pragma once

#include "core/Logger.hpp"
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

  inline bool check_shader(GLuint program, const std::string& name)
  {
    bool ok = true;
    int actual_error_msg_len = 0;
    int status;
    glValidateProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE)
    {
      std::string error_msg(512, '\0');
      glGetProgramInfoLog(program, 512, &actual_error_msg_len, error_msg.data());
      Logger::error("GL_LINK_STATUS failure for shader {}. Error: {}", name, error_msg.substr(0, actual_error_msg_len));
      ok = false;
    }
    glGetProgramiv(program, GL_VALIDATE_STATUS, &status);
    if (status != GL_TRUE)
    {
      std::string error_msg(512, '\0');
      glGetProgramInfoLog(program, 512, &actual_error_msg_len, error_msg.data());
      Logger::error("GL_VALIDATE_STATUS failure for shader {}. Error: {}", name, error_msg.substr(0, actual_error_msg_len));
      ok = false;
    }
    return ok;
  }
}
