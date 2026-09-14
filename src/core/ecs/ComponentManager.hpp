#pragma once

#include "ComponentArray.hpp"
#include "core/Serialization.hpp"
#include "core/ISerializable.hpp"
#include <unordered_map>
#include <string>
#include <memory>
#include <ranges>

namespace fury
{
  namespace ecs
  {
    class ComponentManager : public ISerializable
    {
    public:
      void write(std::ofstream& ofs) const override
      {
        const uint32_t n = static_cast<uint32_t>(m_component_arrays.size());
        BinarySerializer::write(ofs, &n);
        for (const auto& [name, arr] : m_component_arrays)
        {
          const uint32_t components_array_type = arr->get_component_array_type();
          BinarySerializer::write(ofs, &components_array_type);
          BinarySerializer::write(ofs, &name);
          arr->write(ofs);
        }
      }

      void read(std::ifstream& ifs) override
      {
        m_component_arrays.clear();
        uint32_t n = 0;
        BinarySerializer::read(ifs, &n);
        for (uint32_t i = 0; i < n; i++)
        {
          uint32_t components_array_type = 0;
          std::string components_array_name;
          BinarySerializer::read(ifs, &components_array_type);
          BinarySerializer::read(ifs, &components_array_name);
          IComponentArray* parr = reinterpret_cast<IComponentArray*>(ObjectsRegistry::create(components_array_type));
          parr->read(ifs);
          m_component_arrays.insert({ components_array_name, std::unique_ptr<IComponentArray>(parr) });
        }
      }

      template<typename T>
      T& add_component(EntityId entity)
      {
        ComponentArray<T>& ca = get_component_array<T>();
        ca.add(entity, T {});
        return ca.get(entity);
      }

      template<typename T>
      std::remove_reference_t<T>& add_component(EntityId entity, T&& component)
      {
        using Component = std::remove_reference_t<T>;
        ComponentArray<Component>& ca = get_component_array<Component>();
        ca.add(entity, std::forward<T>(component));
        return ca.get(entity);
      }

      template<typename T>
      void remove_component(EntityId entity)
      {
        ComponentArray<T>& ca = get_component_array<T>();
        ca.remove(entity);
      }

      void clear() { m_component_arrays.clear(); }

      void entity_destroyed(EntityId entity)
      {
        for (auto& [name, component_array_ptr] : m_component_arrays)
        {
          component_array_ptr->remove(entity);
        }
      }

      template<typename T>
      T& get_component(EntityId entity)
      {
        return get_component_array<T>().get(entity);
      }

      template<typename T>
      T& get_or_create_component(EntityId entity)
      {
        auto& arr = get_component_array<T>();
        if (arr.contains(entity))
        {
          return get_component<T>(entity);
        }
        else
        {
          return add_component<T>(entity);
        }
      }

      template<typename T>
      bool has_component(EntityId entity) const
      {
        return get_component_safe<T>(entity) != nullptr;
      }

      template<typename T>
      T* get_component_safe(EntityId entity)
      {
        return get_component_array<T>().get_safe(entity);
      }

      template<typename T>
      const T& get_component(EntityId entity) const
      {
        return get_component_array<T>().get(entity);
      }

      template<typename T>
      const T* get_component_safe(EntityId entity) const
      {
        return get_component_array<T>().get_safe(entity);
      }

      template<typename T>
      const std::vector<EntityId>& get_entities() const
      {
        return get_component_array<T>().get_entities();
      }

      template<typename T>
      const std::vector<T>& get_components() const
      {
        return get_component_array<T>().get_components();
      }

      template<typename T>
      void clear_components()
      {
        return get_component_array<T>().clear();
      }

      template<typename T>
      auto get_components_zipped()
      {
        const auto& entities = get_entities<T>();
        const auto& components = get_components<T>();
        assert(entities.size() == components.size());
        auto range_zipped =
            std::views::iota(size_t {}, entities.size()) |
            std::views::transform([&](size_t i)
                                  { return std::make_pair(entities[i], std::ref(const_cast<T&>(components[i]))); });
        return range_zipped;
      }

      template<typename T>
      const auto get_components_zipped() const
      {
        const auto& entities = get_entities<T>();
        const auto& components = get_components<T>();
        assert(entities.size() == components.size());
        auto range_zipped =
            std::views::iota(size_t {}, entities.size()) |
            std::views::transform([&](size_t i) { return std::forward_as_tuple(entities[i], components[i]); });
        return range_zipped;
      }


    private:
      template<typename T>
      ComponentArray<T>& get_component_array()
      {
        const std::string lookup_name = T::component_name.data();
        if (!m_component_arrays.contains(lookup_name))
        {
          m_component_arrays.insert({ lookup_name, std::make_unique<ComponentArray<T>>() });
        }
        const std::unique_ptr<IComponentArray>& components_array_ptr = m_component_arrays.at(lookup_name);
        return static_cast<ComponentArray<T>&>(*components_array_ptr);
      }

      template<typename T>
      const ComponentArray<T>& get_component_array() const
      {
        return const_cast<ComponentManager*>(this)->get_component_array<T>();
      }

    private:
      std::unordered_map<std::string, std::unique_ptr<IComponentArray>> m_component_arrays;
    };
  }; // namespace ecs
}; // namespace fury
