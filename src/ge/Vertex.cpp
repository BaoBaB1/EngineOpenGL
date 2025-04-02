#include "Vertex.hpp"

namespace fury
{
  Vertex::Vertex() {
    position = normal = glm::vec3();
    color = glm::vec4(1.f);
    texture = glm::vec2();
  }

  Vertex::Vertex(const glm::vec3& _position) {
    position = _position;
    normal = glm::vec3();
    color = glm::vec4(1.f);
    texture = glm::vec2();
  }

  Vertex::Vertex(const glm::vec3& _pos, const glm::vec3& _normal, const glm::vec4& _color, const glm::vec2& _texture)
    : position(_pos), normal(_normal), color(_color), texture(_texture)
  {

  }

  Vertex::Vertex(float x, float y, float z) {
    position = glm::vec3(x, y, z);
    normal = glm::vec3(0.f);
    color = glm::vec4(1.f);
    texture = glm::vec2();
  }

  Vertex Vertex::operator+(const Vertex& other) const {
    return Vertex(position + other.position);
  }

  Vertex Vertex::operator/(float value) const {
    if (value == 0.f)
      return Vertex();
    return Vertex(position / value);
  }

  Vertex Vertex::operator*(float value) const {
    return Vertex(position * value);
  }

  bool Vertex::operator==(const Vertex& other) const {
    return color == other.color && position == other.position && normal == other.normal;
  }
}
