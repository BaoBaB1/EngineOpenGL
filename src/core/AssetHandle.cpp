#include "AssetHandle.hpp"
#include "AssetManager.hpp"
#include "Serialization.hpp"

namespace fury
{
  template<typename T>
  SerializableSizeT BinarySerializerSpec<AssetHandle<T>>::write(std::ofstream& ofs, const AssetHandle<T>* t)
  {
    Logger::trace("Writing asset with id {}", t->id);
    return BinarySerializer::write(ofs, &t->id);
  }

  template<typename T>
  SerializableSizeT BinarySerializerSpec<AssetHandle<T>>::read(std::ifstream& ifs, AssetHandle<T>* t)
  {
    AssetId id;
    const SerializableSizeT bytes_read = BinarySerializer::read(ifs, &id);
    Logger::trace("Reading asset with id {}", id);
    if (id != 0)
    {
      // At this point asset manager must already contain all the data
      *t = AssetManager<T>::get_by_id(id);
    }
    return bytes_read;
  }

  // Explicit instantiation in cpp to avoid circular dependencies in header files...
  template struct BinarySerializerSpec<AssetHandle<Texture2D>>;
  template struct BinarySerializerSpec<AssetHandle<Material>>;
  template struct BinarySerializerSpec<AssetHandle<Mesh>>;

} // namespace fury
