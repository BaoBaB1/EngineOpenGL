#pragma once

#include "core/Common.hpp"
#include "core/Logger.hpp"
#include "core/ISerializable.hpp"
#include "core/Serialization.hpp"

#include <unordered_map>
#include <vector>
#include <fstream>

namespace fury
{
  namespace ecs
  {
    class IComponentArray : public ISerializable
    {
    public:
      virtual uint32_t get_component_array_type() const = 0;
      virtual void remove(EntityId entity) = 0;
      virtual ~IComponentArray() = default;
    };

    template<typename T>
    class ComponentArray : public IComponentArray
    {
    public:
      void write(std::ofstream& ofs) const override
      {
        BinarySerializer::write(ofs, &m_components);
        BinarySerializer::write(ofs, &m_entities);
        BinarySerializer::write(ofs, &m_entity_to_idx_map);
      }

      void read(std::ifstream& ifs) override
      {
        BinarySerializer::read(ifs, &m_components);
        BinarySerializer::read(ifs, &m_entities);
        BinarySerializer::read(ifs, &m_entity_to_idx_map);
      }

      uint32_t get_component_array_type() const override { return T::component_array_id; }

      bool contains(EntityId entity) const { return m_entity_to_idx_map.contains(entity); }

      T& get(EntityId entity) { return m_components[m_entity_to_idx_map.at(entity)]; }

      const T& get(EntityId entity) const { return m_components[m_entity_to_idx_map.at(entity)]; }

      T* get_safe(EntityId entity)
      {
        const auto it = m_entity_to_idx_map.find(entity);
        if (it == m_entity_to_idx_map.end())
        {
          return nullptr;
        }
        return &m_components[it->second];
      }

      const T* get_safe(EntityId entity) const { return const_cast<ComponentArray<T>*>(this)->get_safe(entity); }

      bool add(EntityId entity, const T& component) { return add_impl(entity, component); }

      bool add(EntityId entity, T&& component) { return add_impl(entity, std::move(component)); }

      void clear()
      {
        m_components.clear();
        m_entities.clear();
        m_entity_to_idx_map.clear();
      }

      const std::vector<EntityId>& get_entities() const { return m_entities; }

      const std::vector<T>& get_components() const { return m_components; }

      void remove(EntityId entity) override
      {
        const auto it = m_entity_to_idx_map.find(entity);
        if (it == m_entity_to_idx_map.end())
        {
          return;
        }
        assert(!m_components.empty());
        assert(m_components.size() == m_entities.size());
        const size_t component_to_remove_idx = it->second;
        const size_t last_component_idx = m_components.size() - 1;
        const EntityId last_entity_id = m_entities.back();
        m_components[component_to_remove_idx] = std::move(m_components[last_component_idx]);
        m_entities[component_to_remove_idx] = m_entities[last_component_idx];
        m_components.pop_back();
        m_entities.pop_back();
        m_entity_to_idx_map[last_entity_id] = component_to_remove_idx;
        m_entity_to_idx_map.erase(entity);
      }

    private:
      template<typename U, std::enable_if_t<std::is_same_v<std::decay_t<T>, std::decay_t<U>>, int> = 0>
      bool add_impl(EntityId entity, U&& component)
      {
        if (m_entity_to_idx_map.contains(entity))
        {
          Logger::warn("[ComponentArray<T>::add] Entity {} already has component {}.", entity,
                       typeid(component).name());
          return false;
        }
        const size_t idx = m_components.size();
        m_components.push_back(std::forward<U>(component));
        m_entities.push_back(entity);
        m_entity_to_idx_map[entity] = idx;
        return true;
      }
    private:
      std::vector<T> m_components;
      std::vector<EntityId> m_entities;
      std::unordered_map<EntityId, size_t> m_entity_to_idx_map;
    };
  } // namespace ecs

}; // namespace fury
