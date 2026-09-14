#pragma once

#include "utils/Utils.hpp"
#include "core/ObjectsRegistry.hpp"
#include "core/ecs/ComponentArray.hpp"
#include "Common.hpp"

struct TypeInfo
{
  uint32_t id;
  const TypeInfo* base_info = nullptr;
};

#define FURY_OnlyMovable(classname) \
  classname(const classname&) = delete; \
  classname& operator=(const classname&) = delete; \
  classname(classname&&) noexcept = default; \
  classname& operator=(classname&&) noexcept = default;

#define FURY_IF0(x)
#define FURY_IF1(x) x
#define FURY_ELSE0(x) x
#define FURY_ELSE1(x)
#define FURY_IF(check, action) FURY_IF##check(action)
#define FURY_IFELSE(check, action, action2) FURY_IF##check(action) FURY_ELSE##check(action2)

#define FURY_REGISTER_COMMON(cls) \
  constexpr static std::string_view get_class_name() \
  { \
    return cls_name; \
  } \
  constexpr static uint32_t get_static_type_id() \
  { \
    return cls_id; \
  } \
  inline constexpr static std::string_view cls_name = #cls; \
  inline constexpr static uint32_t cls_id = fury::utils::FNV1a32(cls_name); \
  inline static int unused_dummy = fury::ObjectsRegistry::register_type<cls>(cls_id); \
  virtual uint32_t get_dynamic_type_id() const \
  { \
    return cls_id; \
  } \
  virtual const TypeInfo* get_type_info() const \
  { \
    return &cls_type_info; \
  } \
  bool is_a(uint32_t other_type_id) const \
  { \
    const TypeInfo* ti = get_type_info(); \
    while (ti) \
    { \
      if (ti->id == other_type_id) \
      { \
        return true; \
      } \
      ti = ti->base_info; \
    } \
    return false; \
  } \
  using SelfT = cls; \
  friend class ObjectsRegistry;

#define FURY_REGISTER_BASE_CLASS(cls) \
  FURY_REGISTER_COMMON(cls) \
  inline static const TypeInfo cls_type_info = { cls_id, nullptr };

#define FURY_REGISTER_DERIVED_CLASS(cls, base) \
  FURY_REGISTER_COMMON(cls) \
  inline static const TypeInfo cls_type_info = { cls_id, &base::cls_type_info }; \
  using BaseCls = base;

#define FURY_PROPERTY(name, type, field) \
  void set_##name(type name) \
  { \
    field = name; \
  } \
  type get_##name() const \
  { \
    return field; \
  }

#define FURY_PROPERTY_REF(name, type, field) \
  void set_##name(const type& name) \
  { \
    field = name; \
  } \
  const type& get_##name() const \
  { \
    return field; \
  } \
  type& get_##name() \
  { \
    return field; \
  }

#define FURY_REGISTER_COMPONENT(component_type) \
  inline constexpr static uint32_t component_id = fury::utils::FNV1a32(#component_type); \
  inline constexpr static std::string_view component_name = #component_type; \
  inline constexpr static uint32_t component_array_id = fury::utils::FNV1a32("ComponentArray" #component_type); \
  inline static int unused_dummy = fury::ObjectsRegistry::register_type<component_type>(component_id); \
  inline static int unused_dummy2 = \
      fury::ObjectsRegistry::register_template_class<fury::ecs::ComponentArray, component_type>(component_array_id);
