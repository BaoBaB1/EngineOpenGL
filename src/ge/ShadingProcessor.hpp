#pragma once

#include "ge/Vertex.hpp"
#include "ge/Mesh.hpp"
#include <unordered_map>
#include <vector>

using GLuint = unsigned int;

class ShadingProcessor
{
public:
  enum ShadingMode
  {
    NO_SHADING,
    FLAT_SHADING,
    SMOOTH_SHADING,
    LAST_ITEM
  };
  static void apply_shading(std::vector<Mesh>& meshes, ShadingMode mode);
  static void calc_normals(Mesh& mesh, ShadingMode mode); 
private:
  struct WrappedVertex
  {
    explicit WrappedVertex(const Vertex& vertex)
    {
      this->vertex = vertex;
    }
    bool operator==(const WrappedVertex& other) const { return other.vertex.position == this->vertex.position; }
    Vertex vertex;
  };
  struct VertexHasher
  {
    size_t operator()(const WrappedVertex& wrapped) const
    {
      std::hash<float> hasher;
      return hasher(wrapped.vertex.position.x) + hasher(wrapped.vertex.position.y) ^ hasher(wrapped.vertex.position.z);
    }
  };
  struct VertexFinder
  {
    using iter = std::unordered_map<WrappedVertex, GLuint, VertexHasher, std::equal_to<WrappedVertex>>::iterator;
    iter end() { return m_map_vert.end(); }
    iter find_vertex(const Vertex& v)
    {
      return m_map_vert.find(WrappedVertex(v));
    }
    void add_vertex(const Vertex& v, GLuint index)
    {
      m_map_vert.insert(std::make_pair(WrappedVertex(v), index));
    }
    std::unordered_map<WrappedVertex, GLuint, VertexHasher, std::equal_to<WrappedVertex>> m_map_vert; // vertex, index
  };
private:
  static VertexFinder vertex_finder;
};