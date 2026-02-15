#include "RotationController.hpp"
#include "SceneGraphManager.hpp"
#include "Entity.hpp"

namespace fury
{
  RotationController::RotationController(const glm::vec3& axis, float angle)
  {
    m_axis = axis;
    m_angle = angle;
  }

  void RotationController::tick(float dt)
  {
    if (!m_enabled || !m_entity)
    {
      return;
    }
    if (auto node = SceneGraphManager::get_entity_node<TransformationSceneNode>(m_entity->get_id()))
    {
      const glm::quat& q = node->get_rotation();
      node->set_rotation(glm::rotate(q, m_angle * dt, m_axis));
    }
  }
}
