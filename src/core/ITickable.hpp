#pragma once

namespace fury
{
	class ITickable
	{
	public:
		virtual void tick(float dt) = 0;
		virtual ~ITickable() = default;
	};
}
