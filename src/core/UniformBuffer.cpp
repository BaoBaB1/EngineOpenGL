#include "UniformBuffer.hpp"

UniformBuffer::UniformBuffer() : OpenGLBuffer(GL_UNIFORM_BUFFER)
{
}

UniformBuffer::UniformBuffer(size_t size) : OpenGLBuffer(GL_UNIFORM_BUFFER, size)
{
}

void UniformBuffer::set_binding_point(int point) const
{
	glBindBufferBase(GL_UNIFORM_BUFFER, point, id());
}
