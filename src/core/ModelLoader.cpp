#include "ModelLoader.hpp"

std::optional<Object3D> ModelLoader::load(const std::string& filename, unsigned int flags)
{
  Assimp::Importer importer;
  const aiScene* scene = importer.ReadFile(filename, flags);

  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
  {
    DEBUG("Failed to load file " << filename << ". Assimp error " << importer.GetErrorString() << '\n');
    return std::nullopt;
  }
  else
  {
    Object3D model;
    // calc extent to scale all vertices in range [-1, 1]
    calc_max_extent(scene->mRootNode, scene);
    size_t vcount = 0, fcount = 0;
    process(scene->mRootNode, scene, std::filesystem::path(filename).parent_path(), model, vcount, fcount);
    center_around_origin(model);
    // TODO: fix this hack
    model.set_shading_mode(Object3D::SMOOTH_SHADING);
    return model;
  }
}

void ModelLoader::process(const aiNode* root, const aiScene* scene, const std::filesystem::path& file_path, Object3D& model, size_t& vcount, size_t& fcount)
{
  for (unsigned int i = 0; i < root->mNumMeshes; i++)
  {
    const aiMesh* inmesh = scene->mMeshes[root->mMeshes[i]];
    Mesh& outmesh = model.emplace_mesh();
    
    const bool has_normals = inmesh->HasNormals();
    const bool has_texture_coords = inmesh->HasTextureCoords(0);

    // process vertices
    vcount += inmesh->mNumVertices;
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
        v.texture = glm::vec2(inmesh->mTextureCoords[0][vidx].x, 1 - inmesh->mTextureCoords[0][vidx].y);
      }
      outmesh.append_vertex(v);
    }

    // process faces
    assert(inmesh->mNumFaces > 0);
    fcount += inmesh->mNumFaces;
    for (unsigned int fidx = 0; fidx < inmesh->mNumFaces; fidx++)
    {
      aiFace face = inmesh->mFaces[fidx];
      // all must be triangulated for now
      assert(face.mNumIndices == 3);
      Face myface(face.mNumIndices);
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

      const unsigned int ambient_tex_count = ai_material->GetTextureCount(aiTextureType_AMBIENT);
      const unsigned int diffuse_tex_count = ai_material->GetTextureCount(aiTextureType_DIFFUSE);
      const unsigned int specular_tex_count = ai_material->GetTextureCount(aiTextureType_SPECULAR);

      if (ambient_tex_count)
      {
        aiString path;
        ai_material->GetTexture(aiTextureType_AMBIENT, 0, &path);
        outmesh.set_texture(std::make_shared<Texture2D>(file_path / path.C_Str()), TextureType::AMBIENT);
      }

      if (diffuse_tex_count)
      {
        aiString path;
        ai_material->GetTexture(aiTextureType_DIFFUSE, 0, &path);
        outmesh.set_texture(std::make_shared<Texture2D>(file_path / path.C_Str()), TextureType::DIFFUSE);
      }

      if (specular_tex_count)
      {
        aiString path;
        ai_material->GetTexture(aiTextureType_SPECULAR, 0, &path);
        outmesh.set_texture(std::make_shared<Texture2D>(file_path / path.C_Str()), TextureType::SPECULAR);
      }

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
    process(root->mChildren[i], scene, file_path, model, vcount, fcount);
  }

  if (root == scene->mRootNode)
  {
    std::cout << "Model has been loaded. Vertex count: " << vcount << ", face count " << fcount << '\n';
  }
}

void ModelLoader::calc_max_extent(const aiNode* root, const aiScene* scene)
{
  for (unsigned int i = 0; i < root->mNumMeshes; i++)
  {
    const aiMesh* inmesh = scene->mMeshes[root->mMeshes[i]];
    const auto& bbox = inmesh->mAABB;
    const float max_extent = get_max_extent(bbox.mMin, bbox.mMax);
    m_max_extent = std::max(m_max_extent, max_extent);
  }
  for (unsigned int i = 0; i < root->mNumChildren; i++)
  {
    calc_max_extent(root->mChildren[i], scene);
  }
}

float ModelLoader::get_max_extent(const aiVector3D& min, const aiVector3D& max)
{
  const float x = std::max(std::abs(max.x), std::abs(min.x));
  const float y = std::max(std::abs(max.y), std::abs(min.y));
  const float z = std::max(std::abs(max.z), std::abs(min.z));
  return std::max(x, std::max(y, z));
} 

void ModelLoader::center_around_origin(Object3D& model)
{
  // to make outlining work properly if model's origin is at it's base and not 0,0,0
  model.calculate_bbox();
  const auto& bbox = model.bbox();
  glm::vec3 bbox_center = (bbox.min() + bbox.max()) * 0.5f;
  bbox_center /= m_max_extent;
  if (bbox_center != glm::vec3(0.f))
  {
    for (size_t i = 0; i < model.mesh_count(); i++)
    {
      Mesh& mesh = model.get_mesh(i);
      for (Vertex& v : mesh.vertices())
      {
        v.position -= bbox_center;
      }
    }
  }
  // also offset bbox bounds according to new vertex positions
  model.bbox().init(bbox.min() - bbox_center, bbox.max() - bbox_center);
}
