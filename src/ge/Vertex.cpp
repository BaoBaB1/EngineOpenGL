#include "Vertex.hpp"

namespace fury
{
  Vertex::Vertex(const glm::vec3& position_) : position(position_)
  {
  }

  Vertex::Vertex(const glm::vec3& position_, const glm::vec3& normal_, const glm::vec4& color_, const glm::vec2& uv_)
    : position(position_), normal(normal_), color(color_), uv(uv_)
  {
  }

  Vertex::Vertex(float x, float y, float z) : position(glm::vec3(x, y, z))
  {
  }
}
