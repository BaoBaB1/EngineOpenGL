#include "Entity.hpp"
#include "IdGenerator.hpp"
#include "EntityManager.hpp"
#include "SceneGraphManager.hpp"

namespace fury
{
  Entity::Entity(const char* name)
  {
    m_id = IdGeneratorT<Entity>::get_next();
    m_name = name ? name : ("Entity" + std::to_string(m_id));
  }

  Entity::Entity(Entity&& other) noexcept
  {
    move_swap(std::move(other));
  }

  Entity& Entity::operator=(Entity&& other) noexcept
  {
    if (this != &other)
    {
      move_swap(std::move(other));
    }
    return *this;
  }

  SceneNode* Entity::attach_node(SceneNode* node)
  {
    SceneNode* stored_node = SceneGraphManager::store(m_id, node);
    stored_node->set_owner(this);
    return stored_node;
  }

  SceneNode* Entity::attach_node(uint32_t node_rtti_id)
  {
    if (ObjectsRegistry::contains(node_rtti_id))
    {
      SceneNode* node = static_cast<SceneNode*>(ObjectsRegistry::create(node_rtti_id));
      return Entity::attach_node(node);
    }
    else
    {
      Logger::warn("Entity::attach_node: scene nodes` object registry does not contain node with id {}.", node_rtti_id);
      return nullptr;
    }
  }

  void Entity::release()
  {
    EntityManager::remove_entity(m_id);
    SceneGraphManager::remove_entity_nodes(m_id);
    m_id = 0;
  }

  void Entity::move_swap(Entity&& other)
  {
    release();
    // now this corresponds to same id and not the object that is about to be moved
    EntityManager::replace_reference(other.m_id, this);
    for (SceneNode* node : SceneGraphManager::get_entity_nodes(other.m_id))
    {
      node->set_owner(this);
    }
    m_id = other.m_id;
    m_name = std::move(other.m_name);
    other.m_id = 0;
  }

  Entity::~Entity()
  {
    // if we didn't move it
    if (m_id > 0)
    {
      release();
    }
  }
}
