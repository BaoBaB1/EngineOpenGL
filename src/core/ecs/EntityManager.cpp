#include "EntityManager.hpp"
#include "core/IdGenerator.hpp"

namespace fury
{
  namespace ecs
  {
    EntityId EntityManager::create()
    {
      static IdGenerator<EntityId> id_generator;
      EntityId id = id_generator.get_next();
      m_entities.push_back(id);
      return id;
    }

    const std::vector<EntityId>& EntityManager::get_entities() const
    {
      return m_entities;
    }

    bool EntityManager::exists(EntityId entity) const
    {
      return std::find(m_entities.begin(), m_entities.end(), entity) != m_entities.end();
    }

    void EntityManager::clear()
    {
      m_entities.clear();
    }

    void EntityManager::remove(EntityId entity)
    {
      std::erase(m_entities, entity);
    }
  }; // namespace ecs
} // namespace fury
