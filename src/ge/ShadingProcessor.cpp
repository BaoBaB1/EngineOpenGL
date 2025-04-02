#include "ShadingProcessor.hpp"

namespace fury
{
  ShadingProcessor::VertexFinder ShadingProcessor::vertex_finder;

  void ShadingProcessor::apply_shading(std::vector<Mesh>& meshes, ShadingMode mode)
  {
    // duplicate each vertex of current mesh for flat shading so each face has its own normal
    if (mode == ShadingMode::FLAT_SHADING)
    {
      for (auto& mesh : meshes)
      {
        vertex_finder.m_map_vert.clear();
        std::vector<GLuint> indices(3);
        std::vector<Face>& faces = mesh.faces(), new_faces;
        assert(faces.size() > 0);
        for (const auto& face : faces)
        {
          assert(face.size == 3);
          for (int i = 0; i < face.size; ++i)
          {
            Vertex vert = mesh.vertices()[face.data[i]];
            VertexFinder::iter iter = vertex_finder.find_vertex(vert);
            // this vertex already present
            if (iter != vertex_finder.end())
            {
              // add copy
              indices[i] = static_cast<GLuint>(mesh.append_vertex(vert));
            }
            else
            {
              indices[i] = face.data[i];
              vertex_finder.add_vertex(vert, indices[i]);
            }
          }
          new_faces.push_back(indices);
        }
        faces = std::move(new_faces);
        calc_normals(mesh, mode);
      }
    }
    // for smooth shading we have to make sure that every vertex is unique as well as it's normal.
    // fragment color will be interpolated between triangle's vertex normals
    // same applies for shading mode = NO_SHADING, except the difference that normals are 0,0,0
    else
    {
      for (auto& mesh : meshes)
      {
        vertex_finder.m_map_vert.clear();
        std::vector<GLuint> indices(3);
        std::vector<Vertex>& vertices = mesh.vertices(), unique_vertices;
        std::vector<Face>& faces = mesh.faces();
        assert(faces.size() > 0);
        for (auto& face : faces)
        {
          indices.resize(face.size);
          for (int i = 0; i < face.size; ++i)
          {
            Vertex vert = vertices[face.data[i]];
            vert.normal = glm::vec3(0.f);
            VertexFinder::iter iter = vertex_finder.find_vertex(vert);
            if (iter != vertex_finder.end())
            {
              face.data[i] = iter->second;
            }
            else
            {
              unique_vertices.push_back(vert);
              face.data[i] = unique_vertices.size() - 1;
              vertex_finder.add_vertex(vert, face.data[i]);
            }
          }
        }
        vertices = std::move(unique_vertices);
        calc_normals(mesh, mode);
      }
    }
  }

  void ShadingProcessor::calc_normals(Mesh& mesh, ShadingMode mode)
  {
    std::vector<Vertex>& vertices = mesh.vertices();
    for (auto& vert : vertices)
    {
      vert.normal = glm::vec3(0.f);
    }
    if (mode == ShadingMode::NO_SHADING)
      return;
    const std::vector<Face>& faces = mesh.faces();
    for (auto& face : faces)
    {
      assert(face.size == 3);
      GLuint ind = face.data[0], ind2 = face.data[1], ind3 = face.data[2];
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
      else if (ShadingMode::FLAT_SHADING)
      {
        vertices[ind].normal = normal;
        vertices[ind2].normal = normal;
        vertices[ind3].normal = normal;
      }
    }
    for (Vertex& v : vertices)
      if (v.normal != glm::vec3())
        v.normal = glm::normalize(v.normal);
  }
}
