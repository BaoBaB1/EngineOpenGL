#pragma once

#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <string_view>
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

template<>
struct fmt::formatter<glm::mat4> : fmt::formatter<std::string>
{
  auto format(const glm::mat4& mat, format_context& ctx) const -> decltype(ctx.out())
  {
    return fmt::format_to(ctx.out(), "\n[{} {} {} {}]\n[{} {} {} {}]\n[{} {} {} {}]\n[{} {} {} {}]",
      mat[0][0], mat[0][1], mat[0][2], mat[0][3], mat[1][0], mat[1][1], mat[1][2], mat[1][3],
      mat[2][0], mat[2][1], mat[2][2], mat[2][3], mat[3][0], mat[3][1], mat[3][2], mat[3][3]);
  }
};

template<>
struct fmt::formatter<std::string_view> : fmt::formatter<std::string>
{
  auto format(std::string_view str, format_context& ctx) const -> decltype(ctx.out())
  {
    return fmt::format_to(ctx.out(), "{}", str.data());
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
