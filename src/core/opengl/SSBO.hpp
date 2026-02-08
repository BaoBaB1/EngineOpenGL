#pragma once

#include "OpenGLBuffer.hpp"

namespace fury
{
	class SSBO : public OpenGLBuffer
	{
	public:
		SSBO();
		SSBO(size_t size);
	};
}
