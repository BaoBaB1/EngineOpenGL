#pragma once

#include "core/Macros.hpp"
#include "core/Event.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

namespace fury
{
  class SceneNode;

  struct NodeDeserializationData
  {
    SceneNode* node_ptr = nullptr;
    uint64_t node_id = 0;
    uint64_t parent_node_id = 0;
    EntityId entity = 0;
    std::vector<uint64_t> children_node_ids;
  };

  class SceneGraphBuilder
  {
  public:
    void build(const std::vector<NodeDeserializationData>& nodes_data);
  };

  inline Event<const NodeDeserializationData&> on_node_read_completed;

  // TODO: rework as flat 1D array
  class SceneNode
  {
  public:
    FURY_REGISTER_BASE_CLASS(SceneNode)
    SceneNode(const SceneNode& other) = delete;
    SceneNode& operator=(const SceneNode& other) = delete;
    SceneNode(SceneNode&& other) noexcept;
    SceneNode& operator=(SceneNode&& other) noexcept;
    void set_parent(SceneNode*);
    void remove_child(SceneNode*);
    void remove_children();
    void add_child(SceneNode*);
    void set_entity(EntityId entity) { m_owner = entity; }
    EntityId get_entity() const { return m_owner; }
    std::vector<SceneNode*>& get_children() { return m_children; }
    SceneNode* find_child(SceneNode*);
    bool has_parent() const { return m_parent != nullptr; }
    bool is_dirty() const { return m_dirty; }
    void mark_dirty();
    SceneNode* get_parent() { return m_parent; }
    const SceneNode* get_parent() const { return m_parent; }
    const std::vector<SceneNode*>& get_children() const { return m_children; }
    virtual void release();
    virtual void update();
    virtual ~SceneNode();
  protected:
    SceneNode(SceneNode* parent = nullptr);
  protected:
    EntityId m_owner = 0;
    bool m_dirty = false;
    SceneNode* m_parent = nullptr;
    std::vector<SceneNode*> m_children;
  };

  class TransformationSceneNode : public SceneNode
  {
  public:
    FURY_REGISTER_DERIVED_CLASS(TransformationSceneNode, SceneNode)
    TransformationSceneNode(SceneNode* parent = nullptr) : SceneNode(parent) {}
    void update() override;
    void set_scale(const glm::vec3& scale);
    void set_translation(const glm::vec3& translation);
    void set_rotation(const glm::quat& quat);
    void set_rotation(const glm::vec3& axis, float degrees);
    void set_world_matrix(const glm::mat4& world_matrix) { m_world_mat = world_matrix; }
    void set_local_matrix(const glm::mat4& local_matrix) { m_local_mat = local_matrix; }
    glm::mat4& get_local_mat() { return m_local_mat; }
    const glm::mat4& get_local_mat() const { return m_local_mat; }
    glm::mat4& get_world_mat() { return m_world_mat; }
    const glm::mat4& get_world_mat() const { return m_world_mat; }
    const glm::vec3& get_scale() const { return m_scale; }
    const glm::vec3& get_translation() const { return m_translation; }
    const glm::quat& get_rotation() const { return m_rotation; }
    FURY_DECLARE_SERIALIZABLE_FIELDS(FURY_SERIALIZABLE_FIELD(1, &TransformationSceneNode::m_scale),
                                     FURY_SERIALIZABLE_FIELD(2, &TransformationSceneNode::m_translation),
                                     FURY_SERIALIZABLE_FIELD(3, &TransformationSceneNode::m_rotation),
                                     FURY_SERIALIZABLE_FIELD(4, &TransformationSceneNode::m_local_mat),
                                     FURY_SERIALIZABLE_FIELD(5, &TransformationSceneNode::m_world_mat))
  private:
    glm::vec3 m_scale = glm::vec3(1);
    glm::vec3 m_translation = glm::vec3(0);
    glm::quat m_rotation;
    // transform relative to parent
    glm::mat4 m_local_mat = glm::mat4(1.f);
    glm::mat4 m_world_mat = glm::mat4(1.f);
  };

  template<>
  struct BinarySerializerSpec<SceneNode>
  {
    static SerializableSizeT write(std::ofstream& ofs, const SceneNode* t);
    static SerializableSizeT read(std::ifstream& ifs, SceneNode* t);
  };

} // namespace fury
