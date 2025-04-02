#pragma once

#include "OpenGLBuffer.hpp"

namespace fury
{
	class SSBO : public OpenGLBuffer
	{
	public:
		OnlyMovable(SSBO)
			SSBO();
		SSBO(size_t size);
	};
}
