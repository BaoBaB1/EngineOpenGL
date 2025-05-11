#pragma once

#include <glm/glm.hpp>

namespace fury
{
  struct Vertex {
    Vertex() = default;
    Vertex(const glm::vec3& position);
    Vertex(float x, float y, float z);
    Vertex(const glm::vec3& pos, const glm::vec3& normal, const glm::vec4& color, const glm::vec2& texture);
    glm::vec3 position = {};
    glm::vec3 normal = {};
    glm::vec4 color = glm::vec4(1.f);
    glm::vec2 uv = {};
  };
}
