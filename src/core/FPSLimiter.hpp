#pragma once

#include <cstdint>
#include <chrono>

namespace fury
{
  class FPSLimiter
  {
  public:
    FPSLimiter() = default;
    FPSLimiter(uint32_t limit);
    void set_limit(uint32_t fps);
    void wait();
    uint32_t get_limit() const { return m_limit; }
    bool has_limit() const { return m_limit != 0; }
  private:
    // 0 - unlimited
    uint32_t m_limit = 0;
    std::chrono::nanoseconds m_start = {};
    std::chrono::nanoseconds m_frametime = {};
  };
}
