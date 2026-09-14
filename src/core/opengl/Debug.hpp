#pragma once  

#include "core/Logger.hpp"
#include "core/Shader.hpp"
#include <glad/glad.h>
#include <string>
#include <string_view>

namespace fury
{
  #define opengl_check_error() opengl_check_error_impl(__FILE__, __LINE__)

  inline constexpr std::string_view opengl_error_to_str(GLenum error)
  {
    switch (error)
    {
    case GL_INVALID_ENUM:
      return "INVALID_ENUM";
    case GL_INVALID_VALUE:
      return "INVALID_VALUE";
    case GL_INVALID_OPERATION:
      return "INVALID_OPERATION";
    case GL_STACK_OVERFLOW:
      return "STACK_OVERFLOW";
    case GL_STACK_UNDERFLOW:
      return "STACK_UNDERFLOW";
    case GL_OUT_OF_MEMORY:
      return "OUT_OF_MEMORY";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
      return "INVALID_FRAMEBUFFER_OPERATION";
    default:
      return "UNKNOWN";
    }
  }

  inline GLenum opengl_check_error_impl(const char* file, int line)
  {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
      Logger::error("OpenGL error {}, file {}, line {}.", opengl_error_to_str(error), file, line);
    }
    return error;
  }

  inline bool opengl_check_shader(GLuint program, const ShaderDescription& desc)
  {
    bool ok = true;
    int actual_error_msg_len = 0;
    int status;

    std::string defines_str;
    for (const auto& define : desc.defines)
    {
      defines_str += define + " ";
    }
    if (!desc.defines.empty())
    {
      defines_str.pop_back();
    }

    glValidateProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE)
    {
      std::string error_msg(512, '\0');
      glGetProgramInfoLog(program, 512, &actual_error_msg_len, error_msg.data());
      Logger::error("GL_LINK_STATUS failure for shader {}. Defines: {}. Error: {}", desc.name, defines_str,
                    error_msg.substr(0, actual_error_msg_len));
      ok = false;
    }
    glGetProgramiv(program, GL_VALIDATE_STATUS, &status);
    if (status != GL_TRUE)
    {
      std::string error_msg(512, '\0');
      glGetProgramInfoLog(program, 512, &actual_error_msg_len, error_msg.data());
      Logger::error("GL_VALIDATE_STATUS failure for shader {}. Defines: {}. Error: {}", desc.name, defines_str,
                    error_msg.substr(0, actual_error_msg_len));
      ok = false;
    }
    return ok;
  }
} // namespace fury
