#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <optional>
#include <filesystem>

#include "ge/ComplexModel.hpp"

class ModelLoader
{
public:
  std::optional<ComplexModel> load(const std::string& filename, unsigned int flags);
private:
  void process(const aiNode* root, const aiScene* scene, const std::filesystem::path& file_path, ComplexModel& model);
  float get_max_extent(const aiVector3D& min, const aiVector3D& max);
  void calc_max_extent(const aiNode* root, const aiScene* scene);
  void center_around_origin(ComplexModel& model);
  float m_max_extent = 0.f;
};
