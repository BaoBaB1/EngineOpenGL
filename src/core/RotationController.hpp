#pragma once

#include "ObjectController.hpp"
#include <glm/glm.hpp>

namespace fury
{
  class RotationController : public ObjectController
  {
  public:
    RotationController() : ObjectController(Type::ROTATION) {}
    RotationController(const glm::vec3& axis, float angle);
    [[nodiscard]] ObjectController* clone() const override;
    void read(std::ifstream& ifs) override;
    void write(std::ofstream& ofs) const override;
    void tick() override;
    void set_rotation_axis(const glm::vec3& axis) { m_axis = axis; }
    void set_rotation_angle(float angle) { m_angle = angle; }
    float get_rotation_angle() const { return m_angle; }
    glm::vec3 get_rotation_axis() const { return m_axis; }
  private:
    float m_angle = 0.f;
    glm::vec3 m_axis = glm::vec3(0.f);
  };
}
