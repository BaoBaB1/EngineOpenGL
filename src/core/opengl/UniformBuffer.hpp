#pragma once

#include "OpenGLBuffer.hpp"

namespace fury
{
	class UniformBuffer : public OpenGLBuffer
	{
	public:
		OnlyMovable(UniformBuffer)
			UniformBuffer();
		UniformBuffer(size_t size);
	};
}
