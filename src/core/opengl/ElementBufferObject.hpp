#pragma once

#include "OpenGLBuffer.hpp"

namespace fury
{
	class ElementBufferObject : public OpenGLBuffer
	{
	public:
		ElementBufferObject();
		ElementBufferObject(size_t size);
	};
}
