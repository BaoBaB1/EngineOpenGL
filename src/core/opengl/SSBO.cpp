#include "SSBO.hpp"

SSBO::SSBO() : OpenGLBuffer(GL_SHADER_STORAGE_BUFFER)
{
}

SSBO::SSBO(size_t size) : OpenGLBuffer(GL_SHADER_STORAGE_BUFFER, size)
{
}
