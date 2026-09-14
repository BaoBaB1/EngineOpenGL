#pragma once

#include <type_traits>
#include <fstream>
#include <vector>
#include <array>
#include <memory>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>

namespace fury
{
  template<typename T>
  struct FieldPointerTraits;

  template<typename T, typename U>
  struct FieldPointerTraits<T U::*>
  {
    using FieldType = T;
    using ClassType = U;
  };

  template<typename T, typename = void>
  struct IsRegisteredClass : std::false_type
  {
  };

  template<typename T>
  struct IsRegisteredClass<T, std::void_t<decltype(T::cls_id)>> : std::true_type
  {
  };

  template<typename T, typename = void>
  struct HasAnySerializableField : std::false_type
  {
  };

  template<typename T>
  struct HasAnySerializableField<T, std::void_t<decltype(T::get_serializable_fields())>> : std::true_type
  {
  };

  template<typename T, typename = void>
  struct HasBaseClass : std::false_type
  {
  };

  template<typename T>
  struct HasBaseClass<T, std::void_t<typename T::BaseCls>> : std::true_type
  {
  };

  template<typename T>
  struct IsStdVector : std::false_type
  {
  };

  template<typename T>
  struct IsStdVector<std::vector<T>> : std::true_type
  {
  };

  template<typename T>
  constexpr bool IsStdVector_v = IsStdVector<T>::value;

  template<typename T>
  struct IsStdArray : std::false_type
  {
  };

  template<typename T, size_t N>
  struct IsStdArray<std::array<T, N>> : std::true_type
  {
  };

  template<typename T>
  constexpr bool IsStdArray_v = IsStdArray<T>::value;

  template<typename T>
  struct IsStdUnorderedMap : std::false_type
  {
  };

  template<typename K, typename V>
  struct IsStdUnorderedMap<std::unordered_map<K, V>> : std::true_type
  {
  };

  template<typename T>
  constexpr bool IsStdUnorderedMap_v = IsStdUnorderedMap<T>::value;

  template<typename T>
  struct IsStdMap : std::false_type
  {
  };

  template<typename K, typename V>
  struct IsStdMap<std::map<K, V>> : std::true_type
  {
  };

  template<typename T>
  constexpr bool IsStdMap_v = IsStdMap<T>::value;

  template<typename T>
  struct IsStdSet : std::false_type
  {
  };

  template<typename T>
  struct IsStdSet<std::set<T>> : std::true_type
  {
  };

  template<typename T>
  constexpr bool IsStdSet_v = IsStdSet<T>::value;

  template<typename T>
  struct IsStdUnorderedSet : std::false_type
  {
  };

  template<typename T>
  struct IsStdUnorderedSet<std::unordered_set<T>> : std::true_type
  {
  };

  template<typename T>
  constexpr bool IsStdUnorderedSet_v = IsStdUnorderedSet<T>::value;

  template<typename T>
  struct IsStdUniquePointer : std::false_type
  {
  };

  template<typename T>
  struct IsStdUniquePointer<std::unique_ptr<T>> : std::true_type
  {
  };

  template<typename T>
  struct IsStdSharedPointer : std::false_type
  {
  };

  template<typename T>
  struct IsStdSharedPointer<std::shared_ptr<T>> : std::true_type
  {
  };

  template<typename T>
  constexpr bool IsStdSharedPointer_v = IsStdSharedPointer<T>::value;

  template<typename T>
  constexpr bool IsStdUniquePointer_v = IsStdUniquePointer<T>::value;

  template<typename T>
  constexpr bool HasSerializableFields()
  {
    if constexpr (!HasAnySerializableField<T>::value)
    {
      return false;
    }
    if constexpr (HasBaseClass<T>::value)
    {
      if constexpr (HasAnySerializableField<typename T::BaseCls>::value)
      {
        return !std::is_same_v<decltype(T::get_serializable_fields()), decltype(T::BaseCls::get_serializable_fields())>;
      }
    }
    return true;
  }

  template<typename T>
  constexpr bool HasBaseClassSerializableFields()
  {
    if constexpr (HasBaseClass<T>::value)
    {
      return HasSerializableFields<typename T::BaseCls>();
    }
    return false;
  }
} // namespace fury
