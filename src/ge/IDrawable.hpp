#pragma once

#include <string>

class IDrawable
{
public:
  virtual bool has_surface() const = 0;
  virtual std::string name() const = 0;
  virtual ~IDrawable() = default;
};
