#include "FPSLimiter.hpp"
#include "Logger.hpp"

namespace fury
{
  FPSLimiter::FPSLimiter(uint32_t fps)
  {
    set_limit(fps);
  }

  void FPSLimiter::set_limit(uint32_t fps)
  {
    if (fps > 0 && fps < 30)
    {
      Logger::warn("Can't set fps cap {}. Setting fps cap to 30 frames.", fps);
      m_limit = 30;
    }
    else
    {
      m_limit = fps;
    }
    const float frametime = (m_limit == 0) ? 0.f : 1.f / m_limit;
    m_start = std::chrono::steady_clock::now().time_since_epoch();
    m_frametime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<float>(frametime));
  }

  void FPSLimiter::wait()
  {
    if (!has_limit())
      return;
    while (std::chrono::steady_clock::now().time_since_epoch() < m_start + m_frametime)
    {
      std::this_thread::yield();
    }
    //m_start += m_frametime;
    m_start = std::chrono::steady_clock::now().time_since_epoch();
  }
}
