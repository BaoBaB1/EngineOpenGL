#pragma once

#include "ecs/components/DrawableComponent.hpp"
#include <filesystem>
#include <vector>

struct aiScene;
struct aiNode;

namespace fury
{
  class ModelLoader
  {
  public:
    [[nodiscard]] std::vector<MeshNode> load(const std::filesystem::path& file);
  private:
    void process(const aiNode* root, const aiScene* scene, const std::filesystem::path& file,
                 std::vector<MeshNode>& mesh_nodes, float vertex_scale, const float vertex_offset[3]);
  };
} // namespace fury
