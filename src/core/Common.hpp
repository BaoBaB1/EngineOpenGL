#pragma once

#include <memory>
#include <array>

namespace fury
{
  struct Material;
  class Mesh;
  class Texture2D;

  template<typename T>
  using Ref = std::shared_ptr<T>;
  using AssetId = size_t;
  using EntityId = uint32_t;
  using FaceIdxT = uint32_t;
  using TriFace = std::array<FaceIdxT, 3>;
  using Face = TriFace;

  template<typename T>
  class AssetManager;
  using TextureManager = AssetManager<Texture2D>;
  using MeshManager = AssetManager<Mesh>;
  using MaterialManager = AssetManager<Material>;

  template<typename T>
  struct BinarySerializerSpec;
  using SerializableSizeT = uint32_t;
  using SerializableTagT = uint16_t;

  constexpr inline EntityId g_invalid_entity_id = 0;

} // namespace fury
