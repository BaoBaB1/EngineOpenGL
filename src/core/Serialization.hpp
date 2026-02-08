#pragma once

#include "Logger.hpp"
#include "EntityManager.hpp"
#include <fstream>
#include <tuple>
#include <array>
#include <type_traits>
#include <string_view>
#include <functional>
#include <map>
#include <set>
#include <utility>

#define FURY_SERIALIZER_MAJOR_VERSION = 1
#define FURY_SERIALIZER_MINOR_VERSION = 0
#define FURY_SERIALIZER_PATCH_VERSION = 0

namespace fury
{
  // address of shared pointer underlying data (used as unique tag to check if we already serialized it or not)
  inline std::set<void*> serialized_sps;
  // unique tag of data was held by shared pointer + address of shared pointer instance that holds shared data
  inline std::map<uintptr_t, void*> deserialized_sps;

  namespace serializer
  {
    inline void prepare_for_serialization() {
      serialized_sps.clear();
      deserialized_sps.clear();
    }
  }
  
  template<typename T>
  struct FieldPointerTraits;

  template<typename T, typename U>
  struct FieldPointerTraits<T U::*> {
    using FieldType = T;
    using ClassType = U;
  };

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

  template<typename T, typename = void>
  struct IsRegisteredClass : std::false_type {};

  template<typename T>
  struct IsRegisteredClass<T, std::void_t<decltype(T::cls_name)>> : std::true_type {};

  template<typename T, typename = void>
  struct HasAnySerializableField : std::false_type {};
  
  template<typename T>
  struct HasAnySerializableField<T, std::void_t<decltype(T::get_serializable_fields())>> : std::true_type {};

  template<typename T, typename = void>
  struct HasWriteMethod : std::false_type {};

  template<typename T>
  struct HasWriteMethod<T, std::void_t<decltype(std::declval<const T&>().write(std::declval<std::ofstream&>()))>> : std::true_type {};

  template<typename T, typename = void>
  struct HasReadMethod : std::false_type {};

  template<typename T>
  struct HasReadMethod<T, std::void_t<decltype(std::declval<T&>().read(std::declval<std::ifstream&>()))>> : std::true_type {};

  template<typename T, typename = void>
  struct HasBaseClass : std::false_type {};

  template<typename T>
  struct HasBaseClass<T, std::void_t<typename T::BaseCls>> : std::true_type {};

  template<typename T>
  struct IsStdVector : std::false_type {};

  template<typename T>
  struct IsStdVector<std::vector<T>> : std::true_type {};

  template<typename T>
  constexpr bool IsStdVector_v = IsStdVector<T>::value;

  template<typename T>
  struct IsStdArray : std::false_type {};

  template<typename T, size_t N>
  struct IsStdArray<std::array<T, N>> : std::true_type {};

  template<typename T>
  constexpr bool IsStdArray_v = IsStdArray<T>::value;

  template<typename T>
  struct IsStdUniquePointer : std::false_type {};

  template<typename T>
  struct IsStdUniquePointer<std::unique_ptr<T>> : std::true_type {};

  template<typename T>
  struct IsStdSharedPointer : std::false_type {};

  template<typename T>
  struct IsStdSharedPointer<std::shared_ptr<T>> : std::true_type {};

  template<typename T>
  constexpr bool IsStdSharedPointer_v = IsStdSharedPointer<T>::value;

  template<typename T>
  constexpr bool IsStdUniquePointer_v = IsStdUniquePointer<T>::value;

  template<typename T>
  struct Serializer
  {
    static uint64_t write(std::ofstream& ofs, const T* obj)
    {
      if constexpr (!HasAnySerializableField<T>::value)
      {
        Logger::warn("Trying to call Serializer<T>::write for class without serializable fields.");
        return 0;
      }

      //Logger::debug("Writing {}", T::cls_name);

      uint64_t total_bytes_written = 0;
      if constexpr (HasSerializableFields<T>())
      {
        const uint8_t serializable_name_len = static_cast<uint8_t>(T::cls_name.size());
        ofs.write(reinterpret_cast<const char*>(&serializable_name_len), sizeof(uint8_t));
        ofs.write(T::cls_name.data(), serializable_name_len);
        // total size
        size_t pos_total_bytes = ofs.tellp();
        // write dummy value and then overwrite it
        ofs.write(reinterpret_cast<const char*>(&total_bytes_written), sizeof(uint64_t));
        total_bytes_written += sizeof(uint8_t) + serializable_name_len + sizeof(uint64_t);

        auto write_field = [&](std::ofstream& ofs, const T* obj, const auto& tup) -> uint32_t {
          uint16_t tag = std::get<0>(tup);
          ofs.write(reinterpret_cast<const char*>(&tag), sizeof(tag));
          // Logger::debug("Writing tag {} of entity {}", tag, T::cls_name);
          // optional<func>
          if (auto write_func = std::get<3>(tup))
          {
            return sizeof(tag) + (*write_func)(obj, ofs);
          }
          auto member_field_ptr = std::get<1>(tup);
          const auto& field = obj->*member_field_ptr;
          return sizeof(tag) + write_helper(ofs, field);
          };

        std::apply([&](auto&&... args) {
          ((total_bytes_written += write_field(ofs, obj, args)), ...);
          }, T::get_serializable_fields());

        size_t tmp = ofs.tellp();
        ofs.seekp(pos_total_bytes, std::ios_base::beg);
        ofs.write(reinterpret_cast<const char*>(&total_bytes_written), sizeof(uint64_t));
        ofs.seekp(tmp, std::ios_base::beg);
      }
      
      if constexpr (HasBaseClass<T>::value)
      {
        if constexpr (HasAnySerializableField<typename T::BaseCls>::value)
        {
          //Logger::debug("Writing base class {} of {}", T::BaseCls::cls_name, T::cls_name);
          total_bytes_written += Serializer<typename T::BaseCls>::write(ofs, obj);
        }
      }
      return total_bytes_written;
    }

    static uint64_t read(std::ifstream& ifs, T* obj)
    {
      if constexpr (!HasAnySerializableField<T>::value)
      {
        Logger::warn("Trying to call Serializer<T>::read for class without serializable fields.");
        return 0;
      }

      // Logger::debug("Reading {}", T::cls_name);
      uint64_t read_bytes = 0;
      if constexpr (HasSerializableFields<T>())
      {
        uint8_t serializable_name_len = 0;
        ifs.read(reinterpret_cast<char*>(&serializable_name_len), sizeof(uint8_t));
        std::string serializable_name(serializable_name_len, ' ');
        ifs.read(serializable_name.data(), serializable_name_len);
        uint64_t chunk_size;
        ifs.read(reinterpret_cast<char*>(&chunk_size), sizeof(uint64_t));
        read_bytes += sizeof(uint8_t) + serializable_name_len + sizeof(uint64_t);
        if (serializable_name != T::cls_name)
        {
          Logger::error("Expected serializable chunk {}, got {} instead.", T::cls_name, serializable_name.c_str());
          // skip chunk remaining bytes
          ifs.seekg(static_cast<uint64_t>(ifs.tellg()) + chunk_size);
          // read the whole chunk
          return chunk_size;
        }

        const auto& fields = T::get_serializable_fields();
        std::vector<uint16_t> present_tags;
        std::apply([&](auto&&... tup) {
          (present_tags.emplace_back(std::get<0>(tup)), ...);
          }, fields);

        // now reading actual chunk data
        const size_t ifs_start_pos = ifs.tellg();
        while (read_bytes < chunk_size)
        {
          uint16_t tag;
          ifs.read(reinterpret_cast<char*>(&tag), sizeof(tag));
          read_bytes += sizeof(tag);
          // Logger::debug("Reading tag {} of entity {}", tag, T::cls_name);
          // no such field. skip it
          if (std::find(present_tags.begin(), present_tags.end(), tag) == present_tags.end())
          {
            // size (uint32) + field bytes
            uint32_t field_size;
            ifs.read(reinterpret_cast<char*>(&field_size), sizeof(field_size));
            if (read_bytes + field_size <= chunk_size)
            {
              read_bytes += field_size;
              // -4 because we already read field size
              ifs.seekg(field_size - 4, std::ios_base::cur);
              Logger::info("Skipping tag {} for class {}. Bytes skipped {}.", tag, T::cls_name, field_size);
              continue;
            }
            else {
              Logger::error("Can't skip tag {} for class {}. Read_bytes + field_size > chunk_size. \
                Read bytes {}, field size {}, chunk size {}.", tag, T::cls_name, read_bytes, field_size, chunk_size);
              break;
            }
          }

          auto read_field = [=, &ifs, &read_bytes] (auto&& tuple) -> bool {
            // find serializable field with that tag among other fields in tuple of tuples
            if (std::get<0>(tuple) == tag)
            {
              if (auto read_func = std::get<2>(tuple))
              {
                read_bytes += (*read_func)(obj, ifs);
              }
              else
              {
                auto field_ptr = std::get<1>(tuple);
                read_bytes += read_helper(ifs, obj->*field_ptr, tag);
              }
              // Logger::debug("Read field bytes {}.", field_size);
              return true;
            }
            return false;
            };

          std::apply([&](auto&&... tup) {
            ((read_field(tup)) || ...);
            }, fields);
        }

        if (read_bytes != chunk_size)
        {
          Logger::error("Failed to read object {}. Chunk size ({}) != read bytes ({}). Skipping chunk...",
            T::cls_name, chunk_size, read_bytes);
          ifs.seekg(ifs_start_pos + chunk_size, std::ios_base::beg);
        }
      }

      if constexpr (HasBaseClass<T>::value)
      {
        if constexpr (HasAnySerializableField<typename T::BaseCls>::value)
        {
          //Logger::debug("Reading base class {}", T::BaseCls::cls_name);
          read_bytes += Serializer<typename T::BaseCls>::read(ifs, obj);
        }
      }

      // TODO: better approach ?
      if constexpr (std::is_base_of_v<Entity, T>) {
        if (!EntityManager::has_entity(obj->m_id))
        {
          EntityManager::add_entity(obj);
        }
      }
      return read_bytes;
    }

private:
    template<typename Field>
    static uint32_t write_helper(std::ofstream& ofs, const Field& field)
    {
      uint32_t written_bytes = 0;
      size_t pos_written_bytes = ofs.tellp();
      ofs.write(reinterpret_cast<const char*>(&written_bytes), sizeof(written_bytes));
      written_bytes += sizeof(written_bytes);

      if constexpr (IsRegisteredClass<Field>::value)
      {
        // dynamic disptach to exact type
        written_bytes += uint32_t(field.write(ofs));
      }
      else if constexpr (IsStdVector_v<Field>)
      {
        const uint32_t num_elems = static_cast<uint32_t>(field.size());
        ofs.write(reinterpret_cast<const char*>(&num_elems), 4);
        written_bytes += 4;
        std::for_each(field.begin(), field.end(), [&](const auto& item) { written_bytes += write_helper(ofs, item); });
      }
      else if constexpr (IsStdArray_v<Field>)
      {
        // fixed size array, we already know num of elements
        std::for_each(field.begin(), field.end(), [&](const auto& item) { written_bytes += write_helper(ofs, item); });
      }
      else if constexpr (IsStdUniquePointer_v<Field>)
      {
        // write underlying obj
        if (field)
        {
          if constexpr (IsRegisteredClass<Field::element_type>::value)
          {
            const uint32_t id = field->get_dynamic_type_id();
            written_bytes += sizeof(id);
            ofs.write(reinterpret_cast<const char*>(&id), sizeof(id));
          }
          written_bytes += write_helper(ofs, *field);
        }
      }
      else if constexpr (IsStdSharedPointer_v<Field>)
      {
        const bool has_data = static_cast<bool>(field);
        ofs.write(reinterpret_cast<const char*>(&has_data), sizeof(bool));
        written_bytes += sizeof(bool);
        if (field)
        {
          const uintptr_t address_tag = reinterpret_cast<uintptr_t>(field.get());
          ofs.write(reinterpret_cast<const char*>(&address_tag), sizeof(address_tag));
          written_bytes += sizeof(address_tag);
          // if we haven't serialized shared pointer underlying data yet
          if (serialized_sps.count(field.get()) == 0)
          {
            if constexpr (IsRegisteredClass<Field::element_type>::value)
            {
              const uint32_t id = field->get_dynamic_type_id();
              ofs.write(reinterpret_cast<const char*>(&id), sizeof(id));
              written_bytes += sizeof(id);
            }
            serialized_sps.insert(field.get());
            written_bytes += write_helper(ofs, *field);
          }
        }
      }
      else if constexpr (std::is_same_v<Field, std::string>)
      {
        const uint32_t size = static_cast<uint32_t>(field.size());
        ofs.write(reinterpret_cast<const char*>(&size), 4);
        ofs.write(reinterpret_cast<const char*>(field.c_str()), size);
        written_bytes += 4;
        written_bytes += static_cast<uint32_t>(field.size());
      }
      else if constexpr (std::is_trivially_copyable_v<Field>)
      {
        ofs.write(reinterpret_cast<const char*>(&field), sizeof(Field));
        written_bytes += sizeof(Field);
      }
      // c array
      else if constexpr (std::is_array_v<Field>)
      {
        using HeldType = std::remove_all_extents_t<Field>;
        constexpr size_t size = std::extent_v<Field>;
        if constexpr (std::is_trivially_copyable_v<HeldType>)
        {
          ofs.write(reinterpret_cast<const char*>(field[0]), sizeof(field[0]) * size);
          written_bytes += sizeof(field[0]) * size;
        }
        else
        {
          for (size_t i = 0; i < size; i++)
          {
            written_bytes += write_helper(ofs, field[i]);
          }
        }
      }
      else if constexpr (HasWriteMethod<Field>::value)
      {
        written_bytes += field.write(ofs);
      }
      else
      {
        // output Field type
        //Field::nothing;
        static_assert(false, "Not supported serialization type");
      }
      size_t tmp = ofs.tellp();
      ofs.seekp(pos_written_bytes, std::ios_base::beg);
      ofs.write(reinterpret_cast<const char*>(&written_bytes), 4);
      ofs.seekp(tmp, std::ios_base::beg);
      return written_bytes;
    }

    template<typename Field>
    static uint32_t read_helper(std::ifstream& ifs, Field& field, uint16_t tag)
    {
      uint32_t read_bytes = 0;
      uint32_t field_size = 0;
      ifs.read(reinterpret_cast<char*>(&field_size), sizeof(field_size));
      read_bytes += sizeof(field_size);

      if constexpr (IsRegisteredClass<Field>::value)
      {
        // dynamic dispatch
        read_bytes += uint32_t(field.read(ifs));
      }
      else if constexpr (IsStdVector_v<Field>)
      {
        uint32_t num_elems;
        ifs.read(reinterpret_cast<char*>(&num_elems), sizeof(uint32_t));
        read_bytes += 4;
        field.resize(num_elems);
        std::for_each(field.begin(), field.end(), [&](auto& item) { read_bytes += read_helper(ifs, item, tag); });
      }
      else if constexpr (IsStdArray_v<Field>)
      {
        std::for_each(field.begin(), field.end(), [&](auto& elem) { read_bytes += read_helper(ifs, elem, tag); });
      }
      else if constexpr (IsStdUniquePointer_v<Field>)
      {
        using HeldType = typename Field::element_type;
        if constexpr (IsRegisteredClass<HeldType>::value)
        {
          uint32_t type_id;
          ifs.read(reinterpret_cast<char*>(&type_id), sizeof(type_id));
          auto obj_ptr = static_cast<HeldType*>(ObjectsRegistry::create(type_id));
          read_bytes += sizeof(type_id);
          field.reset(obj_ptr);
        }
        else
        {
          field = std::make_unique<typename Field::element_type>();
        }
        read_bytes += read_helper(ifs, *field, tag);
      }
      else if constexpr (IsStdSharedPointer_v<Field>)
      {
        bool had_data;
        ifs.read(reinterpret_cast<char*>(&had_data), sizeof(bool));
        read_bytes += sizeof(bool);
        if (had_data)
        {
          uintptr_t shared_resource_tag;
          ifs.read(reinterpret_cast<char*>(&shared_resource_tag), sizeof(shared_resource_tag));
          read_bytes += sizeof(shared_resource_tag);
          // if we haven't created shared instance of this object yet
          if (deserialized_sps.count(shared_resource_tag) == 0)
          {
            if constexpr (IsRegisteredClass<Field::element_type>::value)
            {
              uint32_t type_id;
              ifs.read(reinterpret_cast<char*>(&type_id), sizeof(type_id));
              read_bytes += sizeof(type_id);
              auto obj = static_cast<Field::element_type*>(ObjectsRegistry::create(type_id));
              field.reset(obj);
            }
            else
            {
              field = std::make_shared<Field::element_type>();
            }
            // read into shared pointer directly
            read_bytes += read_helper(ifs, *field, tag);
            deserialized_sps.insert({ shared_resource_tag, &field });
          }
          else
          {
            // we already have shared pointer with same tag. simply copy it.
            field = *static_cast<Field*>(deserialized_sps.at(shared_resource_tag));
          }
        }
      }
      else if constexpr (std::is_same_v<Field, std::string>)
      {
        uint32_t len;
        ifs.read(reinterpret_cast<char*>(&len), sizeof(len));
        read_bytes += sizeof(len);
        field.resize(len);
        ifs.read(field.data(), len);
        read_bytes += len;
      }
      else if constexpr (std::is_trivially_copyable_v<Field>)
      {
        ifs.read(reinterpret_cast<char*>(&field), sizeof(Field));
        read_bytes += sizeof(Field);
      }
      else if constexpr (std::is_array_v<Field>)
      {
        using HeldType = std::remove_all_extents_t<Field>;
        constexpr size_t size = std::extent_v<Field>;
        if constexpr (std::is_trivially_copyable_v<HeldType>)
        {
          ifs.read(reinterpret_cast<char*>(field[0]), sizeof(field[0]) * size);
          read_bytes += sizeof(field[0]) * size;
        }
        else
        {
          for (size_t i = 0; i < size; i++)
          {
            read_bytes += read_helper(ifs, field[i]);
          }
        }
      }
      else if constexpr (HasReadMethod<Field>::value)
      {
        read_bytes += field.read(ifs);
      }
      else
      {
        // output Field type
        // Field::nothing;
        static_assert(false, "Not supported deserialization type");
      }

      if (read_bytes != field_size)
      {
        const char* name = nullptr;
        if constexpr (IsRegisteredClass<Field>::value)
        {
          name = Field::cls_name.data();
        }
        else
        {
          name = T::cls_name.data();
        }
        Logger::critical("Failed to read field with tag {} of object {}. Field size {}, read bytes {}.",
          tag, name, field_size, read_bytes);
        throw std::runtime_error("Critical error");
      }
      return read_bytes;
    }
  };
}
