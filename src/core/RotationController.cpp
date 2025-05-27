#include "RotationController.hpp"
#include "ge/Object3D.hpp"

namespace fury
{
  RotationController::RotationController(const glm::vec3& axis, float angle) : RotationController()
  {
    m_axis = axis;
    m_angle = angle;
  }

  ObjectController* RotationController::clone() const
  {
    RotationController* clone = new RotationController();
    clone->m_axis = m_axis;
    clone->m_angle = m_angle;
    clone->m_enabled = m_enabled;
    return clone;
  }

  void RotationController::read(std::ifstream& ifs)
  {
    ifs.read(reinterpret_cast<char*>(&m_enabled), sizeof(bool));
    ifs.read(reinterpret_cast<char*>(&m_angle), sizeof(float));
    ifs.read(reinterpret_cast<char*>(&m_axis), sizeof(glm::vec3));
  }

  void RotationController::write(std::ofstream& ofs) const
  {
    ofs.write(reinterpret_cast<const char*>(&m_enabled), sizeof(bool));
    ofs.write(reinterpret_cast<const char*>(&m_angle), sizeof(float));
    ofs.write(reinterpret_cast<const char*>(&m_axis), sizeof(glm::vec3));
  }

  void RotationController::tick()
  {
    if (!m_enabled)
      return;
    m_obj->rotate(m_angle * m_dt, m_axis);
  }
}
