#pragma once

#include "AssetHandle.hpp"
#include "Serialization.hpp"
#include "Mesh.hpp"
#include "Material.hpp"

namespace fury
{
  struct MeshNode
  {
    FURY_DECLARE_SERIALIZABLE_FIELDS(FURY_SERIALIZABLE_FIELD(1, &MeshNode::mesh),
                                     FURY_SERIALIZABLE_FIELD(2, &MeshNode::material))
    AssetHandle<Mesh> mesh;
    AssetHandle<Material> material;
  };
} // namespace fury
