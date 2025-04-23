#include "VertexBufferObject.hpp"

namespace fury
{
	VertexBufferObject::VertexBufferObject() : OpenGLBuffer(GL_ARRAY_BUFFER)
	{
	}

	VertexBufferObject::VertexBufferObject(size_t size) : OpenGLBuffer(GL_ARRAY_BUFFER, size)
	{
	}
}
