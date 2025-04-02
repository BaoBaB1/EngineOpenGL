#pragma once

#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <string>

template<>
struct fmt::formatter<glm::vec3> : fmt::formatter<std::string>
{
  auto format(const glm::vec3& vec, format_context& ctx) const -> decltype(ctx.out())
  {
    return fmt::format_to(ctx.out(), "[{}, {}, {}]", vec.x, vec.y, vec.z);
  }
};

template<>
struct fmt::formatter<glm::vec4> : fmt::formatter<std::string>
{
  auto format(const glm::vec4& vec, format_context& ctx) const -> decltype(ctx.out())
  {
    return fmt::format_to(ctx.out(), "[{} {} {} {}]", vec.x, vec.y, vec.z, vec.w);
  }
};

namespace fury
{
  inline int init_logger()
  {
    spdlog::default_logger()->set_level(spdlog::level::debug);
    return 1;
  }

  namespace Logger
  {
    inline int dummy = init_logger();
    using namespace spdlog;
  }
}
