#pragma once

#include "ge/Object3D.hpp"
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <optional>
#include <filesystem>

namespace fury
{
  class ModelLoader
  {
  public:
    std::optional<Object3D> load(const std::string& file, unsigned int flags);
  private:
    void process(const aiNode* root, const aiScene* scene, const std::filesystem::path& file, Object3D& model);
    void calc_max_extent(const aiNode* root, const aiScene* scene);
    float m_max_extent = 0.f;
  };
}
