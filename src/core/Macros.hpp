#pragma once

#include "utils/Utils.hpp"
#include "Serialization.hpp"
#include "core/ObjectsRegistry.hpp"
#include <optional>
#include <functional>

#define FURY_OnlyMovable(classname) \
  classname(const classname&) = delete; \
  classname& operator=(const classname&) = delete; \
  classname(classname&&) noexcept = default; \
  classname& operator=(classname&&) noexcept = default;

#define FURY_GENERATE_READ_WRITE_FUNC_DEFAULT_IMPL \
  virtual uint64_t read(std::ifstream& ifs) \
  { \
    return fury::Serializer<SelfT>::read(ifs, this); \
  } \
  virtual uint64_t write(std::ofstream& ofs) const \
  { \
    return fury::Serializer<SelfT>::write(ofs, this); \
  }

#define FURY_GENERATE_READ_WRITE_FUNC_NO_IMPL \
  virtual uint64_t read(std::ifstream& ifs); \
  virtual uint64_t write(std::ofstream& ofs) const;

#define FURY_IF0(x)
#define FURY_IF1(x) x
#define FURY_ELSE0(x) x
#define FURY_ELSE1(x)
#define FURY_IF(check, action) FURY_IF##check(action)
#define FURY_IFELSE(check, action, action2) FURY_IF##check(action) FURY_ELSE##check(action2)
// FURY_IFELSE(generate_default_impl_read_write_func, FURY_GENERATE_READ_WRITE_FUNC_DEFAULT_IMPL, FURY_GENERATE_READ_WRITE_FUNC_NO_IMPL)

#define FURY_REGISTER_COMMON(cls, generate_default_read_write_impl) \
  constexpr static std::string_view get_class_name() { return cls_name; } \
  constexpr static uint32_t get_static_type_id() { return cls_id; } \
  inline constexpr static std::string_view cls_name = #cls; \
  inline constexpr static uint32_t cls_id = fury::utils::FNV1a32(cls_name); \
  inline static int unused_dummy = fury::ObjectsRegistry::register_type<cls>(cls_id); \
  virtual uint32_t get_dynamic_type_id() const { return cls_id; } \
  using SelfT = cls; \
  friend struct fury::Serializer<SelfT>; \
  FURY_IFELSE(generate_default_read_write_impl, FURY_GENERATE_READ_WRITE_FUNC_DEFAULT_IMPL, FURY_GENERATE_READ_WRITE_FUNC_NO_IMPL)

#define FURY_REGISTER_CLASS(cls) FURY_REGISTER_COMMON(cls, 1)

#define FURY_REGISTER_CLASS_NO_DEFAULT_READ_WRITE_IMPL(cls) FURY_REGISTER_COMMON(cls, 0)

#define FURY_REGISTER_DERIVED_CLASS(cls, base) \
  FURY_REGISTER_CLASS(cls) \
  using BaseCls = base;

#define FURY_REGISTER_DERIVED_CLASS_NO_DEFAULT_IMPL_READ_WRITE_FUNC(cls, base) \
  FURY_REGISTER_CLASS_NO_DEFAULT_READ_WRITE_IMPL(cls) \
  using BaseCls = base;

#define FURY_PROPERTY(name, type, field) \
  void set_##name(type name) { field = ##name; } \
  type get_##name() const { return field; }

#define FURY_PROPERTY_REF(name, type, field) \
  void set_##name(const type& name) { field = ##name; } \
  const type& get_##name() const { return field; } \
  type& get_##name() { return field; }

#define FURY_SERIALIZABLE_FIELD_TUPLE_TYPE(ptr) \
  uint16_t, \
  decltype(ptr), \
  std::optional<std::function<uint32_t(fury::FieldPointerTraits<decltype(ptr)>::ClassType*, std::ifstream& ifs)>>, \
  std::optional<std::function<uint32_t(const fury::FieldPointerTraits<decltype(ptr)>::ClassType*, std::ofstream& ofs)>>


#define FURY_SERIALIZABLE_FIELD(tag, ptr) std::make_tuple<FURY_SERIALIZABLE_FIELD_TUPLE_TYPE(ptr)>(tag, ptr, {}, {})
#define FURY_SERIALIZABLE_FIELD2(tag, ptr, read_func, write_func) std::make_tuple<FURY_SERIALIZABLE_FIELD_TUPLE_TYPE(ptr)>(tag, ptr, read_func, write_func)

#define FURY_DECLARE_SERIALIZABLE_FIELDS(...) \
  static const auto& get_serializable_fields() \
  { \
    static auto fields = std::make_tuple(__VA_ARGS__); \
    return fields; \
  }
