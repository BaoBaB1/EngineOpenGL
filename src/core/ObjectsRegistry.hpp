#pragma once

#include "Logger.hpp"
#include <functional>
#include <unordered_map>
#include <type_traits>

namespace fury
{
  class ObjectsRegistry
  {
  public:
    // return some dummy value to be able to call it during class declaration
    template<typename T>
    static int register_type(uint32_t cls_id)
    {
      if constexpr (std::is_default_constructible_v<T>)
      {
        if (type_map.count(cls_id) != 0) {
          Logger::warn("Type {} has already been registered! Trying to register classes with same names?", T::cls_name);
          return -1;
        }
        type_map.insert({ T::cls_id, []() { return new T; } });
        return 0;
      }
      else {
        return -1;
      }
    }

    static bool contains(uint32_t cls_id) { return type_map.count(cls_id) != 0; }

    [[nodiscard]] static void* create(uint32_t cls_id)
    {
      if (!contains(cls_id))
      {
        Logger::error("ObjectsRegistry::create: failed to create instance with class id {}.", cls_id);
        return nullptr;
      }
      return type_map.at(cls_id)();
    }

  private:
    inline static std::unordered_map<uint32_t, std::function<void* ()>> type_map;
  };


  //template<typename Base>
  //class ObjectsRegistry
  //{
  //public:
  //  // return some dummy value to be able to call it during class declaration
  //  template<typename T>
  //  static int register_type()
  //  {
  //    if constexpr (std::is_default_constructible_v<T>)
  //    {
  //      type_map.insert({ T::cls_id, []() -> Base* { return new T; } });
  //      return 0;
  //    }
  //    else {
  //      return -1;
  //    }

  //  }

  //  static bool contains(uint32_t id) { return type_map.count(id) != 0; }

  //  [[nodiscard]] static Base* create(uint32_t id)
  //  {
  //    if (!contains(id))
  //    {
  //      Logger::error("ObjectsRegistry::create: failed to create polymorphic instance with id {}.", id);
  //      return nullptr;
  //    }
  //    return type_map.at(id)();
  //  }

  //private:
  //  inline static std::unordered_map<uint32_t, std::function<Base*()>> type_map;
  //};
}
