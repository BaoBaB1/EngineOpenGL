#pragma once

#include <limits>

namespace fury
{
  namespace constants
  {
    constexpr float fmin = std::numeric_limits<float>::min();
    constexpr float fmax = std::numeric_limits<float>::max();
    constexpr double PI = 3.141592653589793238463;
    constexpr float  PI_F = 3.14159265358979f;
    constexpr double PI2 = 2 * PI;
    constexpr float PI2_F = 2 * PI_F;
  }
}
