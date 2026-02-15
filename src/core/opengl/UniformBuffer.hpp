#pragma once

#include "OpenGLBuffer.hpp"

namespace fury
{
	class UniformBuffer : public OpenGLBuffer
	{
	public:
		UniformBuffer();
		UniformBuffer(size_t size);
	};
}
