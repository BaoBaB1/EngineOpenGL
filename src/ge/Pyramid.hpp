#pragma once

#include "Object3D.hpp"

namespace fury
{
	class Pyramid : public Object3D {
	public:
		inline constexpr static int32_t type = 3;
		Pyramid();
		int32_t get_type() const override { return type; }
	};
}
