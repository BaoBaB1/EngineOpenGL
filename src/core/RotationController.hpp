#pragma once

#include "ObjectController.hpp"
#include <glm/glm.hpp>

namespace fury
{
  class RotationController : public ObjectController
  {
  public:
    FURY_REGISTER_DERIVED_CLASS(RotationController, ObjectController)
    RotationController() = default;
    RotationController(const glm::vec3& axis, float angle_radians);
    void tick(float dt) override;
    FURY_PROPERTY_REF(rotation_axis, glm::vec3, m_axis)
    FURY_PROPERTY(rotation_angle, float, m_angle)
    FURY_DECLARE_SERIALIZABLE_FIELDS(
      FURY_SERIALIZABLE_FIELD(1, &RotationController::m_angle),
      FURY_SERIALIZABLE_FIELD(2, &RotationController::m_axis)
    )
  private:
    float m_angle = 0.f;
    glm::vec3 m_axis = glm::vec3(0.f);
  };
}
