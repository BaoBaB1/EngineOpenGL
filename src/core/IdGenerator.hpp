#pragma once

#include <atomic>

namespace fury
{
  template<typename T, typename IdType = uint32_t>
  class IdGeneratorT
  {
  public:
    using IdT = IdType;

    static IdT get_next()
    {
      return id++;
    }

    static IdT get_current()
    {
      return id;
    }

    static void reset(uint32_t new_start = 1)
    {
      id = new_start;
    }
  private:
    inline static std::atomic<IdT> id = 1;
  };
}
