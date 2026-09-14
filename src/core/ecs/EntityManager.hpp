#pragma once

#include "core/Common.hpp"
#include "core/Serialization.hpp"
#include <vector>

namespace fury
{
  namespace ecs
  {
    class EntityManager
    {
    public:
      EntityId create();
      bool exists(EntityId entity) const;
      void remove(EntityId entity);
      const std::vector<EntityId>& get_entities() const;
      void clear();
      FURY_DECLARE_SERIALIZABLE_FIELDS(FURY_SERIALIZABLE_FIELD(1, &EntityManager::m_entities))
    private:
      std::vector<EntityId> m_entities;
    };
  } // namespace ecs
} // namespace fury
