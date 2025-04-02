#pragma once

#include "Ray.hpp"

namespace fury
{
	class IRayHittable
	{
	public:
		virtual std::optional<RayHit> hit(const Ray& ray) const = 0;
		virtual ~IRayHittable() = default;
	};
}
