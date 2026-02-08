#include "EntityManager.hpp"
#include "Entity.hpp"
#include "Logger.hpp"

namespace fury
{
  Entity* EntityManager::get_entity(uint32_t id)
  {
    if (auto it = m_entities.find(id); it != m_entities.end())
    {
      return it->second;
    }
    return nullptr;
  }

  bool EntityManager::has_entity(uint32_t id)
  {
    return m_entities.find(id) != m_entities.end();
  }

  bool EntityManager::add_entity(Entity* ent, bool verbose)
  {
    if (m_entities.count(ent->get_id()))
    {
      if (verbose)
      {
        Logger::error("EntityManager::add_entity: entity with id {} already added.", ent->get_id());
      }
      return false;
    }
    m_entities.emplace(ent->get_id(), ent);
    return true;
  }

  bool EntityManager::remove_entity(uint32_t id, bool verbose)
  {
    if (!m_entities.count(id))
    {
      if (verbose)
      {
        Logger::warn("EntityManager::remove_entity: entity with id {} does not exist.", id);
      }
      return false;
    }
    Entity* ent = m_entities.at(id);
    m_entities.erase(id);
    return true;
  }

  void EntityManager::clear()
  {
    m_entities.clear();
  }

  bool EntityManager::replace_reference(uint32_t from, Entity* to)
  {
    if (has_entity(from))
    {
      m_entities[from] = to;
      return true;
    }
    return false;
  }
}
