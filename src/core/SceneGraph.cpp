#include "SceneGraph.hpp"
#include "SceneGraphManager.hpp"
#include "Entity.hpp"
#include "Logger.hpp"
#include <glm/gtx/quaternion.hpp>
#include <algorithm>

namespace fury
{
  SceneNode::SceneNode(SceneNode* parent) : m_parent(parent)
  {
  }

  void SceneNode::set_parent(SceneNode* parent)
  {
    assert(parent);
    assert(!m_parent);
    m_parent = parent;
    parent->add_child(this);
  }

  void SceneNode::remove_child(SceneNode* child)
  {
    auto it = std::find_if(m_children.begin(), m_children.end(), [=](SceneNode* node) { return node == child; });
    if (it != m_children.end())
    {
      m_children.erase(it);
    }
  }

  void SceneNode::add_child(SceneNode* child)
  {
    assert(child);
    if (find_child(child))
    {
      Logger::warn("Trying to add scene node child that has already been added! Ignoring...");
      return;
    }
    m_children.push_back(child);
    //child->m_parent = this;
  }

  SceneNode* SceneNode::find_child(SceneNode* child)
  {
    auto it = std::find_if(m_children.begin(), m_children.end(), [=](SceneNode* node) { return node == child; });
    return (it != m_children.end() ? *it : nullptr);
  }

  void SceneNode::mark_dirty()
  {
    m_dirty = true;
    SceneGraphManager::add_dirty_node(this);
    for (SceneNode* node : m_children)
    {
      node->mark_dirty();
    }
  }

  void SceneNode::release()
  {
    for (SceneNode* child : m_children)
    {
      child->release();
    }
    m_children.clear();
    if (m_parent)
    {
      m_parent->remove_child(this);
      m_parent = nullptr;
    }
  }

  void SceneNode::update()
  {
    for (SceneNode* child : m_children)
    {
      child->update();
    }
  }

  SceneNode::~SceneNode()
  {
    release();
  }

  TransformationSceneNode::TransformationSceneNode(const glm::mat4& local_mat, const glm::mat4& world_mat)
    : m_local_mat(local_mat), m_world_mat(world_mat)
  {
  }

  void TransformationSceneNode::update()
  {
    if (m_dirty)
    {
      m_local_mat = glm::mat4(1.f);
      
      m_local_mat = glm::translate(m_local_mat, m_translation);
      m_local_mat = glm::scale(m_local_mat, m_scale);
      m_local_mat = m_local_mat * glm::toMat4(m_rotation);

      if (m_parent && m_parent->get_dynamic_type_id() == TransformationSceneNode::get_static_type_id())
      {
        m_world_mat = static_cast<TransformationSceneNode*>(m_parent)->get_world_mat() * m_local_mat;
      }
      else
      {
        m_world_mat = m_local_mat;
      }
      SceneNode::update();
      m_dirty = false;
    }
  }

  void TransformationSceneNode::set_scale(const glm::vec3& scale)
  {
    if (m_scale != scale)
    {
      // avoid scale 0
      m_scale.x = std::max(scale.x, 0.0001f);
      m_scale.y = std::max(scale.y, 0.0001f);
      m_scale.z = std::max(scale.z, 0.0001f);
      mark_dirty();
    }
  }

  void TransformationSceneNode::set_translation(const glm::vec3& translation)
  {
    if (m_translation != translation)
    {
      m_translation = translation;
      mark_dirty();
    }
  }

  void TransformationSceneNode::set_rotation(const glm::quat& quat)
  {
    if (m_rotation != quat)
    {
      m_rotation = quat;
      mark_dirty();
    }
  }

  void TransformationSceneNode::set_rotation(const glm::vec3& axis, float degrees)
  {
    if (axis == glm::vec3(0))
    {
      return;
    }
    if (glm::quat q = glm::angleAxis(glm::radians(degrees), axis); q != m_rotation)
    {
      m_rotation = q;
      mark_dirty();
    }
  }
}
