#pragma once

#include "OpenGLBuffer.hpp"

namespace fury
{
	class ElementBufferObject : public OpenGLBuffer
	{
	public:
		OnlyMovable(ElementBufferObject)
		ElementBufferObject();
		ElementBufferObject(size_t size);
	};
}
