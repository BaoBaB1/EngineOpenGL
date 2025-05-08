#pragma once

#include "Object3D.hpp"

namespace fury
{
	class Pyramid : public Object3D {
	public:
		Pyramid();
		uint32_t get_type() const override { return ObjectsRegistry::get_id<Pyramid>(); }
	};
}
