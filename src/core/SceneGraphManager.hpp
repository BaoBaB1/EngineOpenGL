#pragma once

#include "SceneGraph.hpp"
#include "EntityManager.hpp"
#include "Logger.hpp"
#include <set>
#include <fstream>

namespace fury
{
  class SceneGraphManager
  {
  public:

    static void write(std::ofstream& ofs);
    static void read(std::ifstream& ifs);

    static void add_dirty_node(SceneNode* node)
    {
      m_dirty_nodes2.insert(node);
    }

    static void remove_dirty_node(SceneNode* node)
    {
      m_dirty_nodes2.erase(node);
      m_dirty_nodes.erase(node);
    }

    static void clear()
    {
      m_entity_nodes_map.clear();
      m_dirty_nodes.clear();
      m_dirty_nodes2.clear();
    }

    static void clear_dirty_nodes()
    {
      m_dirty_nodes.clear();
      m_dirty_nodes = m_dirty_nodes2;
      m_dirty_nodes2.clear();
    }

    static std::set<SceneNode*>& get_dirty_nodes()
    {
      return m_dirty_nodes;
    }

    static bool remove_entity_nodes(uint32_t id, bool verbose = true)
    {
      if (!m_entity_nodes_map.count(id))
      {
        if (verbose)
        {
          Logger::warn("SceneGraphManager::remove_entity_nodes: entity with id {} has no nodes.", id);
        }
        return false;
      }
      m_entity_nodes_map.erase(id);
      return true;
    }

    static SceneNode* store(uint32_t entity_id, SceneNode* node)
    {
      auto& ptr = m_entity_nodes_map[entity_id].emplace_back(node);
      return ptr.get();
    }

    static void update_nodes_owner(uint32_t old, Entity* new_owner)
    {
      for (auto& node : m_entity_nodes_map[old])
      {
        node->set_owner(new_owner);
      }
    }

    static std::vector<SceneNode*> get_entity_nodes(uint32_t entity)
    {
      std::vector<SceneNode*> nodes;
      auto it = m_entity_nodes_map.find(entity);
      if (it != m_entity_nodes_map.end())
      {
        for (auto& node : it->second)
        {
          nodes.push_back(node.get());
        }
      }
      return nodes;
    }

    static std::vector<uint32_t> get_managed_entities()
    {
      std::vector<uint32_t> ids;
      ids.reserve(m_entity_nodes_map.size());
      for (const auto& [id, _] : m_entity_nodes_map)
      {
        ids.emplace_back(id);
      }
      return ids;
    }

    template<typename Node>
    static Node* get_entity_node(uint32_t id)
    {
      if (auto it = m_entity_nodes_map.find(id); it != m_entity_nodes_map.end())
      {
        for (auto& node : it->second)
        {
          if (node->get_dynamic_type_id() == Node::get_static_type_id())
          {
            return static_cast<Node*>(node.get());
          }
        }
      }
      return nullptr;
    }

  private:
    inline static std::map<uint32_t, std::vector<std::unique_ptr<SceneNode>>> m_entity_nodes_map;
    inline static std::set<SceneNode*> m_dirty_nodes;
    inline static std::set<SceneNode*> m_dirty_nodes2;
  };
}
