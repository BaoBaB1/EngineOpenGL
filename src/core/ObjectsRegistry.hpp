#pragma once

#include "Logger.hpp"
#include "utils/Utils.hpp"
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
      if constexpr (!std::is_abstract_v<T>)
      {
        if (type_map.count(cls_id) != 0)
        {
          Logger::warn("Type {} has already been registered! Trying to register classes with same names?",
                       utils::get_cls_name_helper<T>());
          return -1;
        }
        type_map.insert({ cls_id, []()
                          {
                            return new T;
                          } });
      }
      return 0;
    }

    template<template<typename> typename Class, typename T>
    static int register_template_class(uint32_t cls_id)
    {
      if constexpr (!std::is_abstract_v<T>)
      {
        if (type_map.count(cls_id) != 0)
        {
          Logger::warn("[ObjectsRegistry::register_template_class]: Template class {} with type {} has already been "
                       "registered! Trying to register instantiated template with same type?",
                       utils::get_cls_name_helper<Class<T>>(), utils::get_cls_name_helper<T>());
          return -1;
        }
        type_map.insert({ cls_id, []()
                          {
                            return new Class<T>;
                          } });
      }
      return 0;
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
    inline static std::unordered_map<uint32_t, std::function<void*()>> type_map;
  };
} // namespace fury
