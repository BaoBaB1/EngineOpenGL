#pragma once

#include "ITickable.hpp"
#include "Macros.hpp"

namespace fury
{
  class Entity;
 
  class ObjectController : public ITickable
  {
  public:
    FURY_REGISTER_CLASS(ObjectController)
    FURY_OnlyMovable(ObjectController)
    ObjectController() = default;
    void set_entity(Entity* entity) { m_entity = entity; }
    const Entity* get_entity() const { return m_entity; }
    bool is_enabled() const { return m_enabled; }
    void disable() { m_enabled = false; }
    void enable() { m_enabled = true; }
    FURY_DECLARE_SERIALIZABLE_FIELDS(
      FURY_SERIALIZABLE_FIELD(1, &ObjectController::m_enabled),
      FURY_SERIALIZABLE_FIELD2(2, &ObjectController::m_entity, &ObjectController::read_entity, &ObjectController::write_entity)
    )
  protected:
    Entity* m_entity = nullptr;
    bool m_enabled = true;
  private:
    uint32_t write_entity(std::ofstream& ofs) const;
    uint32_t read_entity(std::ifstream& ifs);
  };
}
