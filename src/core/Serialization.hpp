#pragma once

#include "Common.hpp"
#include "Logger.hpp"
#include "TypeTraits.hpp"
#include "ObjectsRegistry.hpp"
#include "utils/Utils.hpp"

#include <fstream>
#include <tuple>
#include <array>
#include <type_traits>
#include <string_view>
#include <functional>
#include <map>
#include <set>
#include <utility>
#include <algorithm>

#define FURY_SERIALIZER_MAJOR_VERSION = 1
#define FURY_SERIALIZER_MINOR_VERSION = 0
#define FURY_SERIALIZER_PATCH_VERSION = 0

#define FURY_SERIALIZABLE_FIELD(tag, ptr) std::make_tuple<uint16_t, decltype(ptr)>(tag, ptr)

#define FURY_DECLARE_SERIALIZABLE_FIELDS(...) \
  static const auto& get_serializable_fields() \
  { \
    static auto fields = std::make_tuple(__VA_ARGS__); \
    return fields; \
  }

namespace fury
{
  // address of shared pointer underlying data (used as unique tag to check if we already serialized it or not)
  inline std::set<void*> serialized_sps;
  // unique tag of data was held by shared pointer + address of shared pointer instance that holds shared data
  inline std::map<uintptr_t, void*> deserialized_sps;

  namespace serializer
  {
    inline void prepare_for_serialization()
    {
      serialized_sps.clear();
      deserialized_sps.clear();
    }
  } // namespace serializer

  template<typename T>
  concept HasSpecializedSerializer = requires(std::ofstream& ofs, const T* t) {
    { BinarySerializerSpec<T>::write(ofs, t) } -> std::same_as<SerializableSizeT>;
  };

  template<typename T>
  concept HasSpecializedDeserializer = requires(std::ifstream& ifs, T* t) {
    { BinarySerializerSpec<T>::read(ifs, t) } -> std::same_as<SerializableSizeT>;
  };

  struct BinarySerializer
  {
  public:
    template<typename T>
    static SerializableSizeT write(std::ofstream& ofs, const T* obj)
    {
      SerializableSizeT total_bytes_written = 0;
      if constexpr (HasAnySerializableField<T>::value)
      {
        // Logger::debug("Writing {}", T::cls_name);

        size_t pos_total_bytes = ofs.tellp();
        // write dummy value and then overwrite it
        ofs.write(reinterpret_cast<const char*>(&total_bytes_written), sizeof(total_bytes_written));
        total_bytes_written += sizeof(total_bytes_written);

        if constexpr (IsRegisteredClass<T>::value)
        {
          const uint8_t serializable_name_len = static_cast<uint8_t>(T::cls_name.size());
          ofs.write(reinterpret_cast<const char*>(&serializable_name_len), sizeof(serializable_name_len));
          ofs.write(T::cls_name.data(), serializable_name_len);
          total_bytes_written += sizeof(serializable_name_len) + serializable_name_len;
          if constexpr (HasBaseClass<T>::value)
          {
            //  Logger::debug("Writing base class {} of {}", T::BaseCls::cls_name, T::cls_name);
            total_bytes_written += BinarySerializer::write<typename T::BaseCls>(ofs, obj);
          }
        }

        auto write_field = [&](std::ofstream& ofs, const T* obj, const auto& tup) -> uint32_t
        {
          SerializableSizeT bytes_written = 0;
          const uint16_t tag = std::get<0>(tup);
          const size_t field_size_pos = ofs.tellp();
          ofs.write(reinterpret_cast<const char*>(&bytes_written), sizeof(bytes_written));
          ofs.write(reinterpret_cast<const char*>(&tag), sizeof(tag));
          bytes_written += sizeof(tag) + sizeof(bytes_written);
          // Logger::debug("Writing tag {} of entity {}", tag, T::cls_name);
          auto member_field_ptr = std::get<1>(tup);
          const auto& field = obj->*member_field_ptr;
          bytes_written += write_helper(ofs, field);
          // Write field size in bytes
          const size_t tmp_cur_pos = ofs.tellp();
          ofs.seekp(field_size_pos, std::ios_base::beg);
          ofs.write(reinterpret_cast<const char*>(&bytes_written), sizeof(bytes_written));
          ofs.seekp(tmp_cur_pos, std::ios_base::beg);
          return bytes_written;
        };

        std::apply([&](auto&&... args) { ((total_bytes_written += write_field(ofs, obj, args)), ...); },
                   T::get_serializable_fields());

        const size_t cur_pos_tmp = ofs.tellp();
        ofs.seekp(pos_total_bytes);
        ofs.write(reinterpret_cast<const char*>(&total_bytes_written), sizeof(total_bytes_written));
        ofs.seekp(cur_pos_tmp);
      }
      else
      {
        total_bytes_written += write_helper(ofs, *obj);
      }

      if (total_bytes_written == 0)
      {
        Logger::warn("BinarySerializer<T>::write is called for class {} without serializable fields.",
                     utils::get_cls_name_helper<T>());
      }
      return total_bytes_written;
    }

    template<typename T>
    static SerializableSizeT read(std::ifstream& ifs, T* obj)
    {
      SerializableSizeT read_bytes_total = 0;
      if constexpr (HasAnySerializableField<T>::value)
      {
        // Logger::debug("Reading {}", T::cls_name);

        SerializableSizeT chunk_size;
        ifs.read(reinterpret_cast<char*>(&chunk_size), sizeof(chunk_size));
        read_bytes_total += sizeof(chunk_size);

        if constexpr (IsRegisteredClass<T>::value)
        {
          uint8_t serializable_name_len = 0;
          ifs.read(reinterpret_cast<char*>(&serializable_name_len), sizeof(serializable_name_len));
          std::string serializable_name(serializable_name_len, ' ');
          ifs.read(serializable_name.data(), serializable_name_len);
          read_bytes_total += serializable_name_len + sizeof(uint8_t);
          // TODO: Synonyms/aliases system to be able to deal with the issue when class name has changed
          // but class layout/fields remain the same.
          if (serializable_name != T::cls_name)
          {
            Logger::error("Expected serializable chunk {}, got {} instead.", T::cls_name, serializable_name.c_str());
            // skip chunk remaining bytes
            ifs.seekg(static_cast<uint64_t>(ifs.tellg()) + chunk_size);
            // read the whole chunk
            return chunk_size;
          }
          if constexpr (HasBaseClass<T>::value)
          {
            //  Logger::debug("Reading base class {}", T::BaseCls::cls_name);
            read_bytes_total += BinarySerializer::read<typename T::BaseCls>(ifs, obj);
          }
        }

        const auto& fields = T::get_serializable_fields();
        std::vector<uint16_t> present_tags;
        std::apply([&](auto&&... tup) { (present_tags.emplace_back(std::get<0>(tup)), ...); }, fields);

        // now reading actual chunk data
        const size_t ifs_start_pos = ifs.tellg();
        while (read_bytes_total < chunk_size)
        {
          SerializableSizeT field_bytes = 0;
          SerializableTagT tag = 0;
          ifs.read(reinterpret_cast<char*>(&field_bytes), sizeof(field_bytes));
          ifs.read(reinterpret_cast<char*>(&tag), sizeof(tag));
          const SerializableSizeT field_size = field_bytes - sizeof(tag) - sizeof(field_bytes);
          read_bytes_total += sizeof(tag) + sizeof(field_bytes);
          // Logger::debug("Reading tag {} of entity {}", tag, T::cls_name);
          // no such field. skip it
          if (std::find(present_tags.begin(), present_tags.end(), tag) == present_tags.end())
          {
            // size (uint32) + field bytes
            if (read_bytes_total + field_size <= chunk_size)
            {
              ifs.seekg(field_size, std::ios_base::cur);
              read_bytes_total += field_size;
              Logger::info("Skipping tag {} for class {}. Bytes skipped {}.", tag, utils::get_cls_name_helper<T>(),
                           field_size);
              continue;
            }
            else
            {
              Logger::error("Can't skip tag {} for class {}. Read_bytes + field_size > chunk_size. \
                Read bytes {}, field size {}, chunk size {}.",
                            tag, utils::get_cls_name_helper<T>(), read_bytes_total, field_size, chunk_size);
              break;
            }
          }

          auto read_field = [=, &ifs, &read_bytes_total](auto&& tuple) -> bool
          {
            SerializableSizeT read_field_bytes = 0;
            // find serializable field with that tag among other fields in tuple of tuples
            if (std::get<0>(tuple) == tag)
            {
              auto field_ptr = std::get<1>(tuple);
              read_field_bytes += read_helper(ifs, obj->*field_ptr);
              read_bytes_total += read_field_bytes;
              if (read_field_bytes != field_size)
              {
                Logger::critical(
                    "Failed to read field with tag {} of object {}. Field size read from file {}, read bytes {}.", tag,
                    utils::get_cls_name_helper<T>(), field_size, read_bytes_total);
                throw std::runtime_error("Deserialization critical error");
              }
              // Logger::debug("Read field bytes {}.", field_size);
              return true;
            }
            return false;
          };

          std::apply([&](auto&&... tup) { ((read_field(tup)) || ...); }, fields);
        }

        if (read_bytes_total != chunk_size)
        {
          Logger::error("Failed to read object {}. Chunk size ({}) != read bytes ({}). Skipping chunk...",
                        utils::get_cls_name_helper<T>(), chunk_size, read_bytes_total);
          ifs.seekg(ifs_start_pos + chunk_size, std::ios_base::beg);
        }
      }
      // Otherwise endless recursion
      else
      {
        read_bytes_total += read_helper(ifs, *obj);
      }
      if (read_bytes_total == 0)
      {
        Logger::warn("BinarySerializer<T>::read is called for class {} without serializable fields.",
                     utils::get_cls_name_helper<T>());
      }
      return read_bytes_total;
    }

  private:
    template<typename Field>
    static SerializableSizeT write_helper(std::ofstream& ofs, const Field& field)
    {
      SerializableSizeT written_bytes = 0;
      if constexpr (HasSpecializedSerializer<Field>)
      {
        written_bytes += BinarySerializerSpec<Field>::write(ofs, &field);
      }
      else if constexpr (IsRegisteredClass<Field>::value)
      {
        written_bytes += BinarySerializer::write(ofs, &field);
      }
      else if constexpr (IsStdVector_v<Field>)
      {
        const uint32_t num_elems = static_cast<uint32_t>(field.size());
        ofs.write(reinterpret_cast<const char*>(&num_elems), sizeof(num_elems));
        written_bytes += sizeof(num_elems);
        std::for_each(field.begin(), field.end(), [&](const auto& item) { written_bytes += write_helper(ofs, item); });
      }
      else if constexpr (IsStdArray_v<Field>)
      {
        // fixed size array, we already know num of elements
        std::for_each(field.begin(), field.end(), [&](const auto& item) { written_bytes += write_helper(ofs, item); });
      }
      else if constexpr (IsStdUnorderedMap<Field>::value || IsStdMap<Field>::value)
      {
        const uint32_t map_size = static_cast<uint32_t>(field.size());
        written_bytes += write_helper(ofs, map_size);
        std::for_each(field.begin(), field.end(),
                      [&](const auto& item)
                      {
                        written_bytes += write_helper(ofs, item.first);
                        written_bytes += write_helper(ofs, item.second);
                      });
      }
      else if constexpr (IsStdSet_v<Field> || IsStdUnorderedSet_v<Field>)
      {
        const uint32_t set_size = static_cast<uint32_t>(field.size());
        written_bytes += write_helper(ofs, set_size);
        std::for_each(field.begin(), field.end(), [&](const auto& item) { written_bytes += write_helper(ofs, item); });
      }
      else if constexpr (IsStdUniquePointer_v<Field>)
      {
        // write underlying obj
        if (field)
        {
          if constexpr (IsRegisteredClass<typename Field::element_type>::value)
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
            if constexpr (IsRegisteredClass<typename Field::element_type>::value)
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
        ofs.write(reinterpret_cast<const char*>(&size), sizeof(size));
        ofs.write(reinterpret_cast<const char*>(field.c_str()), size);
        written_bytes += sizeof(size);
        written_bytes += static_cast<uint32_t>(field.size());
      }
      else if constexpr (std::is_same_v<Field, std::filesystem::path>)
      {
        written_bytes += write_helper(ofs, field.string());
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
      else if constexpr (HasAnySerializableField<Field>::value)
      {
        written_bytes += static_cast<uint32_t>(BinarySerializer::write(ofs, &field));
      }
      else
      {
        // output Field type
        // Field::nothing;
        static_assert(utils::always_false<Field>, "Not supported serialization type");
      }
      return written_bytes;
    }

    template<typename Field>
    static SerializableSizeT read_helper(std::ifstream& ifs, Field& field)
    {
      SerializableSizeT read_bytes = 0;
      if constexpr (HasSpecializedDeserializer<Field>)
      {
        read_bytes += static_cast<uint32_t>(BinarySerializerSpec<Field>::read(ifs, &field));
      }
      else if constexpr (IsRegisteredClass<Field>::value)
      {
        read_bytes += static_cast<uint32_t>(BinarySerializer::read(ifs, &field));
      }
      else if constexpr (IsStdVector_v<Field>)
      {
        uint32_t num_elems;
        ifs.read(reinterpret_cast<char*>(&num_elems), sizeof(uint32_t));
        read_bytes += sizeof(num_elems);
        field.resize(num_elems);
        std::for_each(field.begin(), field.end(), [&](auto& item) { read_bytes += read_helper(ifs, item); });
      }
      else if constexpr (IsStdUnorderedMap<Field>::value || IsStdMap<Field>::value)
      {
        uint32_t map_size;
        read_bytes += read_helper(ifs, map_size);
        for (uint32_t i = 0; i < map_size; i++)
        {
          typename Field::key_type key {};
          typename Field::mapped_type value {};
          read_bytes += read_helper(ifs, key);
          read_bytes += read_helper(ifs, value);
          field[key] = std::move(value);
        }
      }
      else if constexpr (IsStdSet_v<Field> || IsStdUnorderedSet_v<Field>)
      {
        uint32_t set_size;
        read_bytes += read_helper(ifs, set_size);
        for (uint32_t i = 0; i < set_size; i++)
        {
          typename Field::key_type value {};
          read_bytes += read_helper(ifs, value);
          field.insert(std::move(value));
        }
      }
      // If array size will ever change, then it will break serialization!!!
      else if constexpr (IsStdArray_v<Field>)
      {
        std::for_each(field.begin(), field.end(), [&](auto& elem) { read_bytes += read_helper(ifs, elem); });
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
        read_bytes += read_helper(ifs, *field);
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
            if constexpr (IsRegisteredClass<typename Field::element_type>::value)
            {
              uint32_t type_id;
              ifs.read(reinterpret_cast<char*>(&type_id), sizeof(type_id));
              read_bytes += sizeof(type_id);
              auto obj = static_cast<Field::element_type*>(ObjectsRegistry::create(type_id));
              field.reset(obj);
            }
            else
            {
              field = std::make_shared<typename Field::element_type>();
            }
            // read into shared pointer directly
            read_bytes += read_helper(ifs, *field);
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
      else if constexpr (std::is_same_v<Field, std::filesystem::path>)
      {
        std::string path;
        read_bytes += read_helper(ifs, path);
        field = path;
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
      else if constexpr (HasAnySerializableField<Field>::value)
      {
        read_bytes += static_cast<uint32_t>(BinarySerializer::read(ifs, &field));
      }
      else
      {
        // output Field type
        Field::nothing;
        static_assert(utils::always_false<Field>, "Not supported deserialization type");
      }
      return read_bytes;
    }
  };
} // namespace fury
