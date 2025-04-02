#pragma once

#include <glm/glm.hpp>

namespace fury
{
  struct Vertex {
    Vertex();
    Vertex(const glm::vec3& position);
    Vertex(float x, float y, float z);
    Vertex(const glm::vec3& pos, const glm::vec3& normal, const glm::vec4& color, const glm::vec2& texture);
    Vertex operator*(float value) const;
    Vertex operator+(const Vertex& other) const;
    Vertex operator/(float value) const;
    bool operator==(const Vertex& other) const;
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec4 color;
    glm::vec2 texture;
  };

  inline Vertex operator*(float factor, const Vertex& v)
  {
    return Vertex(v.position * factor);
  }
}
