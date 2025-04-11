#pragma once

#include <fstream>

namespace fury
{
	class ISerializable
	{
	public:
		virtual void read(std::ifstream&) = 0;
		virtual void write(std::ofstream&) const = 0;
		virtual ~ISerializable() = default;
	};
}
