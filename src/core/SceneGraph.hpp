#pragma once

#include "core/Macros.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

namespace fury
{
  class Entity;

  class SceneNode
  {
  public:
    FURY_REGISTER_CLASS(SceneNode)
    FURY_OnlyMovable(SceneNode)
    Entity* get_owner() { return m_owner; }
    void set_owner(Entity* owner) { m_owner = owner; }
    void set_parent(SceneNode*);
    void remove_child(SceneNode*);
    void add_child(SceneNode*);
    std::vector<SceneNode*>& get_children() { return m_children; }
    SceneNode* find_child(SceneNode*);
    bool has_parent() const { return m_parent != nullptr; }
    bool is_dirty() const { return m_dirty; }
    void mark_dirty();
    SceneNode* get_parent() { return m_parent; }
    virtual void release();
    virtual void update();
    virtual ~SceneNode();
  protected:
    SceneNode(SceneNode* parent = nullptr);
  protected:
    Entity* m_owner = nullptr;
    bool m_dirty = true;
    SceneNode* m_parent = nullptr;
    std::vector<SceneNode*> m_children;
  };

  class TransformationSceneNode : public SceneNode
  {
  public:
    FURY_REGISTER_DERIVED_CLASS(TransformationSceneNode, SceneNode)
    TransformationSceneNode(SceneNode* parent = nullptr) : SceneNode(parent) {}
    TransformationSceneNode(const glm::mat4& local_mat, const glm::mat4& world_mat);
    void update() override;
    void set_scale(const glm::vec3& scale);
    void set_translation(const glm::vec3& translation);
    void set_rotation(const glm::quat& quat);
    void set_rotation(const glm::vec3& axis, float degrees);
    glm::mat4& get_local_mat() { return m_local_mat; }
    const glm::mat4& get_local_mat() const { return m_local_mat; }
    glm::mat4& get_world_mat() { return m_world_mat; }
    const glm::mat4& get_world_mat() const { return m_world_mat; }
    const glm::vec3& get_scale() const { return m_scale; }
    const glm::vec3& get_translation() const { return m_translation; }
    const glm::quat& get_rotation() const { return m_rotation; }
    FURY_DECLARE_SERIALIZABLE_FIELDS(
      FURY_SERIALIZABLE_FIELD(1, &TransformationSceneNode::m_scale),
      FURY_SERIALIZABLE_FIELD(2, &TransformationSceneNode::m_translation),
      FURY_SERIALIZABLE_FIELD(3, &TransformationSceneNode::m_rotation),
      FURY_SERIALIZABLE_FIELD(4, &TransformationSceneNode::m_local_mat),
      FURY_SERIALIZABLE_FIELD(5, &TransformationSceneNode::m_world_mat)
    )
  private:
    glm::vec3 m_scale = glm::vec3(1);
    glm::vec3 m_translation = glm::vec3(0);
    glm::quat m_rotation;
    // transform relative to parent
    glm::mat4 m_local_mat = glm::mat4(1.f);
    glm::mat4 m_world_mat = glm::mat4(1.f);
  };
}
