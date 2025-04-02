#pragma once

#include "OpenGLBuffer.hpp"

namespace fury
{
	class VertexBufferObject : public OpenGLBuffer
	{
	public:
		OnlyMovable(VertexBufferObject)
			VertexBufferObject();
		VertexBufferObject(size_t size);
	};
}
