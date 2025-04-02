#include "UniformBuffer.hpp"

namespace fury
{
	UniformBuffer::UniformBuffer() : OpenGLBuffer(GL_UNIFORM_BUFFER)
	{
	}

	UniformBuffer::UniformBuffer(size_t size) : OpenGLBuffer(GL_UNIFORM_BUFFER, size)
	{
	}
}
