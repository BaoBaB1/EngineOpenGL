#pragma once

#include "Object3D.hpp"

namespace fury
{
	class Cube : public Object3D
	{
	public:
		Cube();
		uint32_t get_type() const override { return ObjectsRegistry::get_id<Cube>(); }
	};
}
