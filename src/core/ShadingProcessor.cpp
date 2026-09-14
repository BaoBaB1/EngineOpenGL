#include "core/Common.hpp"
#include "utils/Utils.hpp"
#include "ShadingProcessor.hpp"
#include <vector>
#include <unordered_map>

template<>
struct std::hash<glm::vec3>
{
  size_t operator()(const glm::vec3& v) const noexcept {
    size_t seed = 0;
    fury::utils::hash_combine(seed, v.x);
    fury::utils::hash_combine(seed, v.y);
    fury::utils::hash_combine(seed, v.z);
    return seed;
  }
};

namespace fury
{
  void ShadingProcessor::apply_shading(Mesh& mesh, ShadingMode mode)
  {
    std::unordered_map<glm::vec3, FaceIdxT> vertex_face_idx_map;

    //  duplicate each vertex of current mesh for flat shading so each face has its own normal
    if (mode == ShadingMode::FLAT_SHADING)
    {
      // GLuint indices[3] = {};
      std::vector<Face>& faces = mesh.get_faces();
      std::vector<Face> new_faces;
      assert(faces.size() > 0);
      for (const auto& face : faces)
      {
        Face& new_face = new_faces.emplace_back();
        for (size_t i = 0; i < face.size(); ++i)
        {
          const Vertex& vertex = mesh.get_vertices()[face[i]];
          auto it = vertex_face_idx_map.find(vertex.position);
          // this vertex already present
          if (it != vertex_face_idx_map.end())
          {
            // add copy
            new_face[i] = static_cast<FaceIdxT>(mesh.append_vertex(vertex));
          }
          else
          {
            new_face[i] = face[i];
            vertex_face_idx_map.emplace(vertex.position, new_face[i]);
          }
        }
      }
      faces = std::move(new_faces);
      calc_normals(mesh, mode);
    }
    // for smooth shading we have to make sure that every vertex is unique as well as it's normal.
    // fragment color will be interpolated between triangle's vertex normals
    // same applies for shading mode = NO_SHADING, except the difference that normals are 0,0,0
    else if (mode == ShadingMode::SMOOTH_SHADING)
    {
      std::vector<uint32_t> indices(3);
      std::vector<Vertex>& vertices = mesh.get_vertices();
      std::vector<Vertex> unique_vertices;
      std::vector<Face>& faces = mesh.get_faces();
      assert(faces.size() > 0);
      for (auto& face : faces)
      {
        indices.resize(face.size());
        for (size_t i = 0; i < face.size(); ++i)
        {
          Vertex vert = vertices[face[i]];
          vert.normal = glm::vec3(0.f);
          auto it = vertex_face_idx_map.find(vert.position);
          //VertexFinder::iter iter = vertex_finder.find_vertex(vert);
          if (it != vertex_face_idx_map.end())
          {
            face[i] = it->second;
          }
          else
          {
            unique_vertices.push_back(vert);
            face[i] = unique_vertices.size() - 1;
            vertex_face_idx_map.emplace(vert.position, face[i]);
          }
        }
      }
      vertices = std::move(unique_vertices);
    }
    calc_normals(mesh, mode);
  }

  void ShadingProcessor::calc_normals(Mesh& mesh, ShadingMode mode)
  {
    std::vector<Vertex>& vertices = mesh.get_vertices();
    for (auto& vert : vertices)
    {
      vert.normal = glm::vec3(0.f);
    }
    if (mode == ShadingMode::NO_SHADING)
    {
      return;
    }
    const std::vector<Face>& faces = mesh.get_faces();
    for (auto& face : faces)
    {
      FaceIdxT ind = face[0], ind2 = face[1], ind3 = face[2];
      glm::vec3 a = vertices[ind2].position - vertices[ind].position;
      glm::vec3 b = vertices[ind3].position - vertices[ind].position;
      glm::vec3 normal = glm::cross(a, b);
      // TODO: some triangles may be in CW order while other in CCW and it affects on normal.
      // so here would be nice somehow check if normal is pointing inside or outside.
      // providing such functionality will avoid defining all faces in CW or CCW order
      // as their normals will always point outside despite their order given in constructor
      // AND/OR make all faces in same winding if there are some in different
      if (mode == ShadingMode::SMOOTH_SHADING)
      {
        vertices[ind].normal += normal;
        vertices[ind2].normal += normal;
        vertices[ind3].normal += normal;
      }
      else if (mode == ShadingMode::FLAT_SHADING)
      {
        vertices[ind].normal = normal;
        vertices[ind2].normal = normal;
        vertices[ind3].normal = normal;
      }
    }
    for (Vertex& v : vertices)
    {
      if (v.normal != glm::vec3())
      {
        v.normal = glm::normalize(v.normal);
      }
    }
  }
} // namespace fury
