#include "SceneGraphManager.hpp"
#include "Entity.hpp"

namespace
{
  using namespace fury;
  struct NodeSerializationInfo
  {
    FURY_REGISTER_CLASS(NodeSerializationInfo)
    // internal node id
    uint32_t id = 0;
    // rtti type id of node
    uint32_t type_id = 0;
    // entity that owns this node
    uint32_t entity_id = 0;
    // parent node id
    uint32_t parent_id = 0;
    // children node ids
    std::vector<uint32_t> children_ids;

    FURY_DECLARE_SERIALIZABLE_FIELDS(
      FURY_SERIALIZABLE_FIELD(1, &NodeSerializationInfo::id),
      FURY_SERIALIZABLE_FIELD(2, &NodeSerializationInfo::type_id),
      FURY_SERIALIZABLE_FIELD(3, &NodeSerializationInfo::entity_id),
      FURY_SERIALIZABLE_FIELD(4, &NodeSerializationInfo::parent_id),
      FURY_SERIALIZABLE_FIELD(5, &NodeSerializationInfo::children_ids)
    )
  };
}

namespace fury
{
  void SceneGraphManager::write(std::ofstream& ofs)
  {
    std::map<SceneNode*, NodeSerializationInfo> nodes_serialization_map;
    size_t num_pos = ofs.tellp();
    uint32_t num_nodes = 0;
    uint32_t id = 1;
    ofs.write(reinterpret_cast<const char*>(&num_nodes), sizeof(num_nodes));
    for (const auto& [entity_id, nodes] : m_entity_nodes_map)
    {
      for (const auto& node_ptr : nodes)
      {
        if (!node_ptr)
        {
          continue;
        }
        NodeSerializationInfo& info = nodes_serialization_map[node_ptr.get()];
        info.id = id++;
        info.type_id = node_ptr->get_dynamic_type_id();
        info.entity_id = node_ptr->get_owner()->get_id();
        num_nodes++;
      }
    }

    for (auto& [node, info] : nodes_serialization_map)
    {
      if (node->has_parent())
      {
        info.parent_id = nodes_serialization_map[node->get_parent()].id;
      }
      for (SceneNode* child : node->get_children())
      {
        info.children_ids.push_back(nodes_serialization_map[child].id);
      }
      Serializer<NodeSerializationInfo>::write(ofs, &info);
      // now write node specific data
      node->write(ofs);
    }
    size_t tmp = ofs.tellp();
    ofs.seekp(num_pos, std::ios_base::beg);
    ofs.write(reinterpret_cast<const char*>(&num_nodes), sizeof(num_nodes));
    ofs.seekp(tmp, std::ios_base::beg);
  }

  void SceneGraphManager::read(std::ifstream& ifs)
  {
    SceneGraphManager::clear();
    std::map<SceneNode*, NodeSerializationInfo> nodes_serialization_map;
    std::map<uint32_t, SceneNode*> id_to_node_map;
    uint32_t num_nodes;
    ifs.read(reinterpret_cast<char*>(&num_nodes), sizeof(num_nodes));

    // read all nodes
    for (int i = 0; i < num_nodes; i++)
    {
      NodeSerializationInfo info;
      Serializer<NodeSerializationInfo>::read(ifs, &info);
      SceneNode* node = static_cast<SceneNode*>(ObjectsRegistry::create(info.type_id));
      node->mark_dirty();
      // read node specific data
      node->read(ifs);
      nodes_serialization_map[node] = info;
      id_to_node_map[info.id] = node;
      Entity* owner = EntityManager::get_entity(info.entity_id);
      owner->attach_node(node);
      //SceneGraphManager::attach_node_to_entity(owner->get_id(), node);
    }
    // resolve parent/child references
    for (auto& [node, info] : nodes_serialization_map)
    {
      if (info.parent_id)
      {
        node->set_parent(id_to_node_map[info.parent_id]);
      }
      for (const uint32_t id : info.children_ids)
      {
        node->add_child(id_to_node_map[id]);
      }
    }
  }
}
