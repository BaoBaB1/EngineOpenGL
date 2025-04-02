#pragma once

namespace fury
{
	class ITickable
	{
	public:
		virtual void tick() = 0;
		virtual ~ITickable() = default;
	};
}
