#include <glm/glm.hpp>
#include <chrono>
#include <utility>
#include "Icosahedron.hpp"
#include "./core/Debug.hpp"

Icosahedron::Icosahedron()
{
  auto& mesh = emplace_mesh();
  mesh.vertices().reserve(12);
  mesh.faces().reserve(20);
  float phi = (1.0f + std::sqrt(5.0f)) * 0.5f; // golden ratio
  float a = 1.0f;
  float b = a / phi;
  // 12 points
  mesh.append_vertex(Vertex(0.f, b, -a));
  mesh.append_vertex(Vertex(b, a, 0.f));
  mesh.append_vertex(Vertex(-b, a, 0.f));
  mesh.append_vertex(Vertex(0.f, b, a));
  mesh.append_vertex(Vertex(0.f, -b, a));
  mesh.append_vertex(Vertex(-a, 0.f, b));
  mesh.append_vertex(Vertex(0.f, -b, -a));
  mesh.append_vertex(Vertex(a, 0.f, -b));
  mesh.append_vertex(Vertex(a, 0.f, b));
  mesh.append_vertex(Vertex(-a, 0.f, -b));
  mesh.append_vertex(Vertex(b, -a, 0.f));
  mesh.append_vertex(Vertex(-b, -a, 0.f));
  // 20 faces
  mesh.append_face(Face({ 2, 1, 0 }));
  mesh.append_face(Face({ 1, 2, 3 }));
  mesh.append_face(Face({ 5, 4, 3 }));
  mesh.append_face(Face({ 4, 8, 3 }));
  mesh.append_face(Face({ 7, 6, 0 }));
  mesh.append_face(Face({ 6, 9, 0 }));
  mesh.append_face(Face({ 11, 10, 4 }));
  mesh.append_face(Face({ 10, 11, 6 }));
  mesh.append_face(Face({ 9, 5, 2 }));
  mesh.append_face(Face({ 5, 9, 11 }));
  mesh.append_face(Face({ 8, 7, 1 }));
  mesh.append_face(Face({ 7, 8, 10 }));
  mesh.append_face(Face({ 2, 5, 3 }));
  mesh.append_face(Face({ 8, 1, 3 }));
  mesh.append_face(Face({ 9, 2, 0 }));
  mesh.append_face(Face({ 1, 7, 0 }));
  mesh.append_face(Face({ 11, 9, 6 }));
  mesh.append_face(Face({ 7, 10, 6 }));
  mesh.append_face(Face({ 5, 11, 4 }));
  mesh.append_face(Face({ 10, 8, 4 }));
}

void Icosahedron::project_points_on_sphere() {
  auto& mesh = get_mesh(0);
  for (Vertex& v : mesh.vertices()) {
    v.position = glm::normalize(v.position);
  }
}

void Icosahedron::subdivide_triangles(int subdivision_level, const Vertex& a, const Vertex& b, const Vertex& c) {
  if (subdivision_level == 0) {
    auto& mesh = get_mesh(0);
    GLuint ind = mesh.append_vertex(a);
    GLuint ind2 = mesh.append_vertex(b);
    GLuint ind3 = mesh.append_vertex(c);
    mesh.append_face(Face({ ind, ind2, ind3 }));
    return;
  }
  Vertex ab((a + b) / 2);
  Vertex bc((b + c) / 2);
  Vertex ac((a + c) / 2);
  ab.color = bc.color = ac.color = m_color;
  // ORDER IS IMPORTANT !!! 
  subdivide_triangles(subdivision_level - 1, a, ab, ac);
  subdivide_triangles(subdivision_level - 1, b, bc, ab);
  subdivide_triangles(subdivision_level - 1, c, ac, bc);
  subdivide_triangles(subdivision_level - 1, ab, bc, ac);
}

void Icosahedron::subdivide_triangles(int subdivision_depth) {
  std::vector<Face> faces = std::move(get_mesh(0).faces());
  std::vector<Vertex> vertices = std::move(get_mesh(0).vertices());
  allocate_memory_before_subdivision(subdivision_depth, (int)faces.size());
  for (size_t i = 0; i < faces.size(); ++i) {
    assert(faces[i].size == 3);
    GLuint index = faces[i].data[0];
    GLuint index2 = faces[i].data[1];
    GLuint index3 = faces[i].data[2];
    subdivide_triangles(subdivision_depth, vertices[index], vertices[index2], vertices[index3]);
  }
}

void Icosahedron::allocate_memory_before_subdivision(int subdivision_depth, int face_count) {
  auto& mesh = get_mesh(0);
  mesh.vertices().clear();
  mesh.faces().clear();
  size_t new_face_count = face_count * (size_t)std::pow(4, subdivision_depth);
  size_t new_vert_count = new_face_count* 3;
  mesh.vertices().reserve(new_vert_count);
  mesh.faces().reserve(new_face_count);
  DEBUG("preallocating memory for " << new_vert_count << " icosahedron points\n");
  DEBUG("preallocating memory for " << new_face_count << " icosahedron faces\n");
}
