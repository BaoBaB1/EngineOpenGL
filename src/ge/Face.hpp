#pragma once

#include "core/Macros.hpp"
#include <glad/glad.h>
#include <array>

namespace fury
{
  template<int N>
  struct FaceN
  {
    using IdxType = GLuint;
    constexpr static int size = N;
    FaceN() = default;
    FaceN(const std::array<GLuint, N>& indices);
    FaceN(const FaceN& other);
    FaceN& operator=(const FaceN& other);
    uint32_t write(std::ofstream& ofs) const;
    uint32_t read(std::ifstream& ifs);
    const uint32_t operator[](GLuint idx) const { return data[idx]; }
    uint32_t operator[](GLuint idx) { return data[idx]; }
    IdxType data[N] = {};
  };
  using Face = FaceN<3>;
  using Face3 = FaceN<3>;
  using Face4 = FaceN<4>;
}
