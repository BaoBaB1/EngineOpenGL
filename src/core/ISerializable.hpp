#pragma once

#include <fstream>

namespace fury
{
  class ISerializable
  {
  public:
    virtual void write(std::ofstream& ofs) const = 0;
    virtual void read(std::ifstream& ifs) = 0;
    virtual ~ISerializable() = default;
  };
} // namespace fury
