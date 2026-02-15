#pragma once

#include "OpenGLBuffer.hpp"

namespace fury
{
	class VertexBufferObject : public OpenGLBuffer
	{
	public:
		VertexBufferObject();
		VertexBufferObject(size_t size);
	};
}
