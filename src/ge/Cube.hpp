#pragma once

#include "Object3D.hpp"

namespace fury
{
	class Cube : public Object3D
	{
	public:
		inline constexpr static int32_t type = 1;
		Cube();
		int32_t get_type() const override { return type; }
	};
}
