#pragma once

#include "Macros.hpp"
#include <string>

namespace fury
{
  class SceneNode;

  class Entity
  {
  public:
    FURY_REGISTER_CLASS(Entity)
    FURY_PROPERTY_REF(name, std::string, m_name)
    Entity(const Entity&) = delete;
    Entity& operator=(const Entity& other) = delete;
    Entity(Entity&& other) noexcept;
    Entity& operator=(Entity&& other) noexcept;
    SceneNode* attach_node(SceneNode* node);
    SceneNode* attach_node(uint32_t id);
    template<typename Node>
    Node* attach_node();
    uint32_t get_id() const { return m_id; }
    virtual ~Entity();
    FURY_DECLARE_SERIALIZABLE_FIELDS(
      FURY_SERIALIZABLE_FIELD(1, &Entity::m_id),
      FURY_SERIALIZABLE_FIELD(2, &Entity::m_name)
    )
  protected:
    Entity(const char* name = nullptr);
    void release();
  protected:
    uint32_t m_id;
    std::string m_name;
  private:
    void move_swap(Entity&& other);
  };

  template<typename Node>
  Node* Entity::attach_node()
  {
    return static_cast<Node*>(attach_node(new Node));
  }
}
