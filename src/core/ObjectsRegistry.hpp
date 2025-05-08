#pragma once

#include "Logger.hpp"
#include <string>
#include <functional>

namespace fury
{
  class Object3D;

  class ObjectsRegistry
  {
  public:
    template<typename T>
    static uint32_t register_type()
    {
      auto& entry = type_map[typeid(T).name()];
      entry.first = id;
      entry.second = []() -> std::unique_ptr<Object3D> { return std::make_unique<T>(); };
      return id++;
    }

    template<typename T>
    static bool contains() { return type_map.count(typeid(T).name()) != 0; }

    template<typename T>
    static uint32_t get_id()
    {
      return type_map.at(typeid(T).name()).first;
    }

    static std::unique_ptr<Object3D> create(uint32_t id)
    {
      for (const auto& [key, val]: type_map)
      {
        if (val.first == id)
        {
          return val.second();
        }
      }
      Logger::warn("Could not find object with id {} in objects registry. Creating Object3D.", id);
      return std::unique_ptr<Object3D>();
    }

  private:
    using FactoryMethod = std::function<std::unique_ptr<Object3D>()>;
    inline static std::unordered_map<std::string, std::pair<uint32_t, FactoryMethod>> type_map;
    inline static uint32_t id = 0;
  };
}
