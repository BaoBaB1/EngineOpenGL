#include "SceneGraph.hpp"
#include "Logger.hpp"
#include <glm/gtx/quaternion.hpp>
#include <algorithm>
#include <unordered_map>

namespace fury
{
  void SceneGraphBuilder::build(const std::vector<NodeDeserializationData>& nodes_data)
  {
    std::unordered_map<uint64_t, SceneNode*> id_node_map;
    for (const auto& data : nodes_data)
    {
      id_node_map.insert({ data.node_id, data.node_ptr });
    }
    for (const auto& data : nodes_data)
    {
      data.node_ptr->set_entity(data.entity);
      if (data.parent_node_id != 0)
      {
        const auto it = id_node_map.find(data.parent_node_id);
        if (it == id_node_map.end())
        {
          Logger::error("Failed to set parent node. Parent node with id {} not found!", data.parent_node_id);
        }
        else
        {
          data.node_ptr->set_parent(it->second);
        }
      }
      for (uint64_t child_id : data.children_node_ids)
      {
        const auto it = id_node_map.find(child_id);
        if (it == id_node_map.end())
        {
          Logger::error("Failed to add child node. Child node with id {} not found!", child_id);
        }
        else
        {
          data.node_ptr->add_child(it->second);
        }
      }
    }
    for (const auto& data : nodes_data)
    {
      data.node_ptr->mark_dirty();
    }
  }

  SceneNode::SceneNode(SceneNode* parent) : m_parent(parent)
  {
  }

  SceneNode::SceneNode(SceneNode&& other) noexcept
  {
    for (SceneNode* child : other.m_children)
    {
      if (child->get_parent() == &other)
      {
        child->m_parent = this;
      }
    }
    if (other.m_parent)
    {
      other.m_parent->remove_child(&other);
      other.m_parent->add_child(this);
    }
    m_children = std::move(other.m_children);
    m_parent = other.m_parent;
    m_dirty = other.m_dirty;
    m_owner = other.m_owner;
    other.m_parent = nullptr;
    other.m_owner = 0;
    other.m_dirty = false;
  }

  SceneNode& SceneNode::operator=(SceneNode&& other) noexcept
  {
    if (this != &other)
    {
      for (SceneNode* child : other.m_children)
      {
        if (child->get_parent() == &other)
        {
          child->m_parent = this;
        }
      }
      if (other.m_parent)
      {
        other.m_parent->remove_child(&other);
        other.m_parent->add_child(this);
      }

      m_children = std::move(other.m_children);
      m_parent = other.m_parent;
      m_dirty = other.m_dirty;
      m_owner = other.m_owner;

      other.m_parent = nullptr;
      other.m_owner = 0;
      other.m_dirty = false;
    }
    return *this;
  }

  void SceneNode::set_parent(SceneNode* parent)
  {
    m_parent = parent;
    if (parent)
    {
      parent->add_child(this);
    }
  }

  void SceneNode::remove_children()
  {
    for (SceneNode* child : m_children)
    {
      remove_child(child);
    }
  }

  void SceneNode::remove_child(SceneNode* child)
  {
    for (size_t i = 0; i < m_children.size(); i++)
    {
      if (m_children[i] == child)
      {
        assert(m_children[i]->m_parent == this);
        m_children.erase(m_children.begin() + i);
        return;
      }
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
    // child->m_parent = this;
  }

  SceneNode* SceneNode::find_child(SceneNode* child)
  {
    auto it = std::find(m_children.begin(), m_children.end(), child);
    return (it != m_children.end() ? *it : nullptr);
  }

  void SceneNode::mark_dirty()
  {
    if (!m_dirty)
    {
      m_dirty = true;
      for (SceneNode* node : m_children)
      {
        node->mark_dirty();
      }
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
    // TODO: now only entity adds and releases nodes from SceneGraphManager
    // maybe SceneNode's dctor should also do that ?
    // SceneGraphManager::remove_node(this);
  }

  void SceneNode::update()
  {
    if (m_parent)
    {
      m_parent->update();
    }
    for (SceneNode* child : m_children)
    {
      child->update();
    }
  }

  SceneNode::~SceneNode()
  {
    release();
  }

  void TransformationSceneNode::update()
  {
    if (m_dirty)
    {
      m_dirty = false;
      m_local_mat = glm::mat4(1.f);
      m_local_mat =
          glm::translate(glm::mat4(1.f), m_translation) * glm::toMat4(m_rotation) * glm::scale(glm::mat4(1.f), m_scale);
      if (m_parent && m_parent->get_dynamic_type_id() == TransformationSceneNode::get_static_type_id())
      {
        m_parent->update();
        m_world_mat = static_cast<TransformationSceneNode*>(m_parent)->get_world_mat() * m_local_mat;
      }
      else
      {
        m_world_mat = m_local_mat;
      }
      SceneNode::update();
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

  SerializableSizeT BinarySerializerSpec<SceneNode>::write(std::ofstream& ofs, const SceneNode* node)
  {
    if (!node) [[unlikely]]
    {
      return 0;
    }
    SerializableSizeT bytes_written = 0;
    const uint64_t node_id = reinterpret_cast<uint64_t>(node);
    const EntityId owner = node->get_entity();
    const uint32_t num_children = static_cast<uint32_t>(node->get_children().size());
    const bool has_parent = node->has_parent();
    bytes_written += BinarySerializer::write(ofs, &node_id);
    bytes_written += BinarySerializer::write(ofs, &has_parent);
    bytes_written += BinarySerializer::write(ofs, &owner);
    bytes_written += BinarySerializer::write(ofs, &num_children);
    if (has_parent)
    {
      const uint64_t parent_node_id = reinterpret_cast<uint64_t>(node->get_parent());
      bytes_written += BinarySerializer::write(ofs, &parent_node_id);
    }
    for (SceneNode* child : node->get_children())
    {
      const uint64_t child_node_id = reinterpret_cast<uint64_t>(child);
      bytes_written += BinarySerializer::write(ofs, &child_node_id);
    }
    return bytes_written;
  }

  SerializableSizeT BinarySerializerSpec<SceneNode>::read(std::ifstream& ifs, SceneNode* node)
  {
    if (!node) [[unlikely]]
    {
      return 0;
    }
    SerializableSizeT bytes_read = 0;
    NodeDeserializationData data;
    uint32_t num_children = 0;
    bool has_parent = false;
    data.node_ptr = node;
    bytes_read += BinarySerializer::read(ifs, &data.node_id);
    bytes_read += BinarySerializer::read(ifs, &has_parent);
    bytes_read += BinarySerializer::read(ifs, &data.entity);
    bytes_read += BinarySerializer::read(ifs, &num_children);
    if (has_parent)
    {
      bytes_read += BinarySerializer::read(ifs, &data.parent_node_id);
    }
    data.children_node_ids.resize(num_children);
    for (uint32_t i = 0; i < num_children; i++)
    {
      bytes_read += BinarySerializer::read(ifs, &data.children_node_ids[i]);
    }
    on_node_read_completed.notify(data);
    return bytes_read;
  }

} // namespace fury
