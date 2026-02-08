#pragma once

#include "Object3D.hpp"

namespace fury
{
	class Pyramid : public Object3D {
	public:
		FURY_REGISTER_DERIVED_CLASS(Pyramid, Object3D)
		Pyramid();
	};
}
