#pragma once

#include "Common.hpp"

namespace fury
{
  template<typename T>
  struct AssetHandle
  {
    explicit operator bool() const noexcept { return asset.get() != nullptr && id != g_invalid_entity_id; }

    AssetId id = 0;
    Ref<T> asset;
  };

  template<typename T>
  struct BinarySerializerSpec<AssetHandle<T>>
  {
    static SerializableSizeT write(std::ofstream& ofs, const AssetHandle<T>* t);
    static SerializableSizeT read(std::ifstream& ifs, AssetHandle<T>* t);
  };

} // namespace fury
