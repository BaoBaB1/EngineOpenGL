#include "VertexBufferObject.hpp"

namespace fury
{
	VertexBufferObject::VertexBufferObject() : VertexBufferObject(GL_ARRAY_BUFFER)
	{
	}

	VertexBufferObject::VertexBufferObject(size_t size) : OpenGLBuffer(GL_ARRAY_BUFFER, size)
	{
	}
}
