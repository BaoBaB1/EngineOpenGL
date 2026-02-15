#pragma once

#include <unordered_map>
#include <string>

namespace fury
{
  class Entity;

  class EntityManager
  {
  public:
    static Entity* get_entity(uint32_t id);
    static bool has_entity(uint32_t id);
    static bool add_entity(Entity* ent, bool verbose = true);
    static bool remove_entity(uint32_t id, bool verbose = true);
    static void clear();
    static bool replace_reference(uint32_t from, Entity* to);
  private:
    inline static std::unordered_map<uint32_t, Entity*> m_entities;
  };
}
