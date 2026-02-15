#include "ObjectController.hpp"
#include "EntityManager.hpp"
#include "Entity.hpp"

namespace fury
{
  uint32_t ObjectController::write_entity(std::ofstream& ofs) const
  {
    uint32_t id = 0;
    if (m_entity)
    {
      id = m_entity->get_id();
    }
    ofs.write(reinterpret_cast<const char*>(&id), sizeof(id));
    return sizeof(id);
  }

  uint32_t ObjectController::read_entity(std::ifstream& ifs)
  {
    uint32_t id;
    ifs.read(reinterpret_cast<char*>(&id), sizeof(id));
    if (id != 0)
    {
      m_entity = EntityManager::get_entity(id);
    }
    return sizeof(id);
  }
}
