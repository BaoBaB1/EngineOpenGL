#include "ModelLoader.hpp"
#include "Logger.hpp"
#include "AssetManager.hpp"
#include "Mesh.hpp"
#include "Common.hpp"
#include "BoundingBox.hpp"
#include "opengl/Texture2D.hpp"
#include <glm/gtx/component_wise.hpp>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace fury
{
  static AssetHandle<Texture2D> read_texture(const aiMaterial* material, const std::filesystem::path& file,
                                             aiTextureType assimp_type);
  static void calc_bbox(const aiNode* root, const aiScene* scene, BoundingBox& bbox);

  std::vector<MeshNode> ModelLoader::load(const std::filesystem::path& file)
  {
    Logger::debug("Starting importing file {}.", file);
    constexpr unsigned int assimp_flags = aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices |
                                          aiProcess_GenBoundingBoxes /*| aiProcess_GenSmoothNormals*/;
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(file.string().c_str(), assimp_flags);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
      Logger::error("Failed to load file {}. Assimp error {}.", file, importer.GetErrorString());
      return {};
    }
    else
    {
      BoundingBox bbox;
      calc_bbox(scene->mRootNode, scene, bbox);
      const float vscale = 1.f / glm::compMax(glm::abs(bbox.max() - bbox.min()));
      const glm::vec3 new_min = bbox.min() * vscale;
      const glm::vec3 new_max = bbox.max() * vscale;
      bbox.init(new_min, new_max);
      const glm::vec3 center = bbox.center();
      // Offset to center model around origin
      const float voffset[3] = { -center.x, -center.y, -center.z };
      std::vector<MeshNode> nodes;
      nodes.reserve(scene->mRootNode->mNumMeshes);
      process(scene->mRootNode, scene, file, nodes, vscale, voffset);
      size_t nvertices = 0, nfaces = 0;
      utils::get_meshes_metadata(nodes, nvertices, nfaces);
      Logger::debug("Loaded model {}. Vertex count {}, face count {}.", file, nvertices, nfaces);
      return nodes;
    }
  }

  void ModelLoader::process(const aiNode* root, const aiScene* scene, const std::filesystem::path& file,
                            std::vector<MeshNode>& mesh_nodes, float vertex_scale, const float vertex_offset[3])
  {
    for (unsigned int i = 0; i < root->mNumMeshes; i++)
    {
      const aiMesh* inmesh = scene->mMeshes[root->mMeshes[i]];
      MeshNode& mesh_node = mesh_nodes.emplace_back();
      Logger::debug("Importing mesh {} of node {}", i, root->mName.C_Str());
      const std::string mesh_lookup_name = std::format("Node{} Mesh{} File{}", root->mName.C_Str(), i, file.string());
      mesh_node.mesh = MeshManager::get_by_name(mesh_lookup_name);
      if (!mesh_node.mesh)
      {
        mesh_node.mesh = MeshManager::create(mesh_lookup_name);
        Mesh& mesh = *mesh_node.mesh.asset;

        const bool has_normals = inmesh->HasNormals();
        const bool has_texture_coords = inmesh->HasTextureCoords(0);

        // process vertices
        for (unsigned int vidx = 0; vidx < inmesh->mNumVertices; vidx++)
        {
          aiVector3D vert = inmesh->mVertices[vidx];
          vert *= vertex_scale;
          vert.x += vertex_offset[0];
          vert.y += vertex_offset[1];
          vert.z += vertex_offset[2];

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
          mesh.append_vertex(v);
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
            myface[i] = face.mIndices[i];
          }
          mesh.append_face(myface);
        }

        // process materials
        if (inmesh->mMaterialIndex < scene->mNumMaterials)
        {
          aiMaterial* ai_material = scene->mMaterials[inmesh->mMaterialIndex];
          aiColor3D ai_color;
          const std::string material_entry_name =
              std::format("Node{} MaterialIdx{} File{}", root->mName.C_Str(), inmesh->mMaterialIndex, file.string());
          Logger::debug("Importing material {} of node {}", inmesh->mMaterialIndex, root->mName.C_Str());
          mesh_node.material = MaterialManager::get_by_name(material_entry_name);
          if (!mesh_node.material)
          {
            mesh_node.material = MaterialManager::create(material_entry_name);
            Material& mat = *mesh_node.material.asset;

            mat.ambient_tex = read_texture(ai_material, file, aiTextureType_AMBIENT);
            mat.diffuse_tex = read_texture(ai_material, file, aiTextureType_DIFFUSE);
            mat.specular_tex = read_texture(ai_material, file, aiTextureType_SPECULAR);

            if (ai_material->Get(AI_MATKEY_COLOR_DIFFUSE, ai_color) == aiReturn_SUCCESS)
            {
              mat.diffuse.r = ai_color.r;
              mat.diffuse.g = ai_color.g;
              mat.diffuse.b = ai_color.b;
            }

            if (ai_material->Get(AI_MATKEY_COLOR_AMBIENT, ai_color) == aiReturn_SUCCESS)
            {
              mat.ambient.r = ai_color.r;
              mat.ambient.g = ai_color.g;
              mat.ambient.b = ai_color.b;
            }

            if (ai_material->Get(AI_MATKEY_COLOR_SPECULAR, ai_color) == aiReturn_SUCCESS)
            {
              mat.specular.r = ai_color.r;
              mat.specular.g = ai_color.g;
              mat.specular.b = ai_color.b;
            }

            float shininess;
            if (ai_material->Get(AI_MATKEY_SHININESS, shininess) == aiReturn_SUCCESS && shininess > 0)
            {
              mat.shininess = shininess;
            }
          }
        }
      }
    }

    for (unsigned int i = 0; i < root->mNumChildren; i++)
    {
      process(root->mChildren[i], scene, file, mesh_nodes, vertex_scale, vertex_offset);
    }
  }

  void calc_bbox(const aiNode* root, const aiScene* scene, BoundingBox& bbox)
  {
    for (unsigned int i = 0; i < root->mNumMeshes; i++)
    {
      const aiMesh* inmesh = scene->mMeshes[root->mMeshes[i]];
      bbox.grow(glm::vec3(inmesh->mAABB.mMin.x, inmesh->mAABB.mMin.y, inmesh->mAABB.mMin.z),
                glm::vec3(inmesh->mAABB.mMax.x, inmesh->mAABB.mMax.y, inmesh->mAABB.mMax.z));
    }
    for (unsigned int i = 0; i < root->mNumChildren; i++)
    {
      calc_bbox(root->mChildren[i], scene, bbox);
    }
  }

  AssetHandle<Texture2D> read_texture(const aiMaterial* material, const std::filesystem::path& file,
                                      aiTextureType assimp_tex_type)
  {
    const unsigned int tex_count = material->GetTextureCount(assimp_tex_type);
    if (tex_count)
    {
      if (tex_count > 1)
      {
        Logger::warn("File {} has {} {} textures, but only first one will be taken.", file.string(), tex_count,
                     aiTextureTypeToString(assimp_tex_type));
      }
      // tex_path is relative to file's folder
      aiString tex_path;
      material->GetTexture(assimp_tex_type, 0, &tex_path);
      const std::filesystem::path full_tex_path = file.parent_path() / tex_path.C_Str();
      return TextureManager::load(full_tex_path);
    }
    return {};
  }
} // namespace fury
