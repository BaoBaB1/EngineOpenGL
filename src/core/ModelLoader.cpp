#include "ModelLoader.hpp"
#include "Logger.hpp"
#include "AssetManager.hpp"
#include "TextureManager.hpp"
#include <assimp/Importer.hpp>

namespace
{
  void center_around_origin(fury::Object3D& model);
  void read_textures(fury::Mesh& mesh, const aiMaterial* material, const std::filesystem::path& file, 
    aiTextureType assimp_type, fury::TextureType engine_type);
  void add_assets(const std::filesystem::path& file, const fury::Object3D& model);
  float get_max_extent(const aiVector3D& min, const aiVector3D& max);
}

namespace fury
{
  bool ModelLoader::load(const std::string& file, unsigned int flags, Object3D& model)
  {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(file, flags);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
      Logger::error("Failed to load file {}. Assimp error {}.", file, importer.GetErrorString());
      return false;
    }
    else
    {
      // calc extent to scale all vertices in range [-1, 1]
      calc_max_extent(scene->mRootNode, scene);
      const std::filesystem::path filepath = std::filesystem::path(file);
      process(scene->mRootNode, scene, filepath, model);
      ::center_around_origin(model);
      // set some shading mode so that textures will be applied (if present) during rendering
      model.set_shading_mode(ShadingProcessor::ShadingMode::SMOOTH_SHADING);
      model.set_is_fixed_shading(true);
      ::add_assets(filepath, model);
      return true;
    }
  }

  void ModelLoader::process(const aiNode* root, const aiScene* scene, const std::filesystem::path& file, Object3D& model)
  {
    for (unsigned int i = 0; i < root->mNumMeshes; i++)
    {
      const aiMesh* inmesh = scene->mMeshes[root->mMeshes[i]];
      Mesh& outmesh = model.emplace_mesh();

      const bool has_normals = inmesh->HasNormals();
      const bool has_texture_coords = inmesh->HasTextureCoords(0);

      // process vertices
      for (unsigned int vidx = 0; vidx < inmesh->mNumVertices; vidx++)
      {
        aiVector3D vert = inmesh->mVertices[vidx];
        vert /= m_max_extent;

        Vertex v;
        v.position = glm::vec3(vert.x, vert.y, vert.z);
        if (has_normals)
        {
          v.normal = glm::vec3(inmesh->mNormals[vidx].x, inmesh->mNormals[vidx].y, inmesh->mNormals[vidx].z);
        }
        if (has_texture_coords)
        {
          v.uv = glm::vec2(inmesh->mTextureCoords[0][vidx].x, 1 - inmesh->mTextureCoords[0][vidx].y);
        }
        outmesh.append_vertex(v);
      }

      // process faces
      assert(inmesh->mNumFaces > 0);
      for (unsigned int fidx = 0; fidx < inmesh->mNumFaces; fidx++)
      {
        aiFace face = inmesh->mFaces[fidx];
        // all must be triangulated for now
        assert(face.mNumIndices == 3);
        Face myface;
        for (unsigned int i = 0; i < face.mNumIndices; i++)
        {
          myface.data[i] = face.mIndices[i];
        }
        outmesh.append_face(myface);
      }

      // process materials
      if (inmesh->mMaterialIndex < scene->mNumMaterials)
      {
        aiMaterial* ai_material = scene->mMaterials[inmesh->mMaterialIndex];
        aiColor3D ai_color;
        Material material;

        ::read_textures(outmesh, ai_material, file, aiTextureType_AMBIENT, fury::TextureType::AMBIENT);
        ::read_textures(outmesh, ai_material, file, aiTextureType_DIFFUSE, fury::TextureType::DIFFUSE);
        ::read_textures(outmesh, ai_material, file, aiTextureType_SPECULAR, fury::TextureType::SPECULAR);

        if (ai_material->Get(AI_MATKEY_COLOR_DIFFUSE, ai_color) == aiReturn_SUCCESS)
        {
          material.diffuse.r = ai_color.r;
          material.diffuse.g = ai_color.g;
          material.diffuse.b = ai_color.b;
        }

        if (ai_material->Get(AI_MATKEY_COLOR_AMBIENT, ai_color) == aiReturn_SUCCESS)
        {
          material.ambient.r = ai_color.r;
          material.ambient.g = ai_color.g;
          material.ambient.b = ai_color.b;
        }

        if (ai_material->Get(AI_MATKEY_COLOR_SPECULAR, ai_color) == aiReturn_SUCCESS)
        {
          material.specular.r = ai_color.r;
          material.specular.g = ai_color.g;
          material.specular.b = ai_color.b;
        }

        float shininess;
        if (ai_material->Get(AI_MATKEY_SHININESS, shininess) == aiReturn_SUCCESS)
        {
          material.shininess = shininess;
        }
        outmesh.set_material(material);
      }
    }

    for (unsigned int i = 0; i < root->mNumChildren; i++)
    {
      process(root->mChildren[i], scene, file, model);
    }

    if (root == scene->mRootNode)
    {
      ObjectGeometryMetadata meta = model.get_geometry_metadata();
      Logger::info("Model has been loaded. Vertex count: {}, face count {}", meta.vert_count_total, meta.face_count_total);
    }
  }

  void ModelLoader::calc_max_extent(const aiNode* root, const aiScene* scene)
  {
    for (unsigned int i = 0; i < root->mNumMeshes; i++)
    {
      const aiMesh* inmesh = scene->mMeshes[root->mMeshes[i]];
      const float max_extent = ::get_max_extent(inmesh->mAABB.mMin, inmesh->mAABB.mMax);
      m_max_extent = std::max(m_max_extent, max_extent);
    }
    for (unsigned int i = 0; i < root->mNumChildren; i++)
    {
      calc_max_extent(root->mChildren[i], scene);
    }
  }
}

namespace
{
  using namespace fury;
  void center_around_origin(Object3D& model)
  {
    // to make outlining work properly if model's origin is at it's base and not 0,0,0
    model.calculate_bbox();
    const auto& bbox = model.get_bbox();
    glm::vec3 bbox_center = bbox.center();
    if (bbox_center != glm::vec3(0.f))
    {
      for (Mesh& mesh : model.get_meshes())
      {
        for (Vertex& v : mesh.vertices())
        {
          v.position -= bbox_center;
        }
      }
    }
    // also offset bbox bounds according to new vertex positions
    model.get_bbox().init(bbox.min() - bbox_center, bbox.max() - bbox_center);
  }

  void read_textures(fury::Mesh& mesh, const aiMaterial* material, const std::filesystem::path& file, 
    aiTextureType assimp_tex_type, fury::TextureType engine_tex_type)
  {
    const unsigned int tex_count = material->GetTextureCount(assimp_tex_type);
    if (tex_count)
    {
      if (tex_count > 1)
      {
        Logger::warn("File {} has {} {} textures, but only first one is taken.", file.string(), tex_count, aiTextureTypeToString(assimp_tex_type));
      }
      // tex_path is relative to file's folder
      aiString tex_path;
      material->GetTexture(assimp_tex_type, 0, &tex_path);
      auto tex = TextureManager::get(file.parent_path() / tex_path.C_Str());
      tex->set_type(engine_tex_type);
      mesh.set_texture(tex, engine_tex_type);
    }
  }

  void add_assets(const std::filesystem::path& file, const fury::Object3D& model)
  {
    // add assets (put all referenced files in same 'assets/filename' folder)
    const std::string filename = file.stem().string();
    const std::filesystem::path mtlpath = file.parent_path() / (filename + ".mtl");
    if (std::filesystem::exists(mtlpath))
    {
      // check if there is mtl file with same name
      AssetManager::add(mtlpath.string(), filename);
    }
    AssetManager::add(file, filename);
    for (const Mesh& m : model.get_meshes())
    {
      for (int i = 0; i < static_cast<int>(TextureType::LAST); i++)
      {
        if (auto tex = m.get_texture(static_cast<TextureType>(i)))
        {
          AssetManager::add(tex->get_file(), filename);
        }
      }
    }
  }

  float get_max_extent(const aiVector3D& min, const aiVector3D& max)
  {
    const float x = std::max(std::abs(max.x), std::abs(min.x));
    const float y = std::max(std::abs(max.y), std::abs(min.y));
    const float z = std::max(std::abs(max.z), std::abs(min.z));
    return std::max(x, std::max(y, z));
  }
}
