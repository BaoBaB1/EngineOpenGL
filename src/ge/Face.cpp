#include "Face.hpp"

namespace fury
{
  template<int N>
  FaceN<N>::FaceN(const FaceN& other) {
    std::memcpy(data, other.data, sizeof(GLuint) * N);
  }

  template<int N>
  FaceN<N>& FaceN<N>::operator=(const FaceN& other) {
    if (this != &other)
    {
      std::memcpy(data, other.data, sizeof(GLuint) * N);
    }
    return *this;
  }

  template<int N>
  FaceN<N>::FaceN(const std::array<GLuint, N>& indices)
  {
    std::memcpy(data, indices.data(), sizeof(GLuint) * N);
  }

  template struct FaceN<3>;
  // TODO: do i even need it ?
  template struct FaceN<4>;
}
