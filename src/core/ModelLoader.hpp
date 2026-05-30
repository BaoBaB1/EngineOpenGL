#pragma once

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <filesystem>
#include <memory>

namespace fury
{
  class Object3D;
  class ModelLoader
  {
  public:
    std::unique_ptr<Object3D> load(const std::string& file, unsigned int flags);
  private:
    void process(const aiNode* root, const aiScene* scene, const std::filesystem::path& file, Object3D* model);
    void calc_max_extent(const aiNode* root, const aiScene* scene);
    float m_max_extent = 0.f;
  };
} // namespace fury
