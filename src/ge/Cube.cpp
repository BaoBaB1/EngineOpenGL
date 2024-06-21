#include "Cube.hpp"

Cube::Cube()
{
  auto& mesh = m_meshes[0];
  mesh.vertices().reserve(24);
  mesh.faces().reserve(12);
  for (int i = 0; i < 3; ++i) {
    mesh.append_vertex(Vertex(0.f, 0.f, 0.f)); 
    mesh.append_vertex(Vertex(1.f, 0.f, 0.f)); 
    mesh.append_vertex(Vertex(1.f, 1.f, 0.f)); 
    mesh.append_vertex(Vertex(0.f, 1.f, 0.f)); 
    mesh.append_vertex(Vertex(0.f, 0.f, 1.f));
    mesh.append_vertex(Vertex(1.f, 0.f, 1.f));
    mesh.append_vertex(Vertex(1.f, 1.f, 1.f));
    mesh.append_vertex(Vertex(0.f, 1.f, 1.f));
  }
  // back
  mesh.append_face(Face{ 1, 0, 3 });
  mesh.append_face(Face{ 1, 3, 2 });
  // front
  mesh.append_face(Face{ 4, 5, 6 });
  mesh.append_face(Face{ 4, 6, 7 });
  // bottom
  mesh.append_face(Face{ 8, 9, 13 });
  mesh.append_face(Face{ 8, 13, 12 });
  // top
  mesh.append_face(Face{ 15, 14, 10 });
  mesh.append_face(Face{ 15, 10, 11 });
  // left
  mesh.append_face(Face{ 16, 20, 23 });
  mesh.append_face(Face{ 16, 23, 19 });
  // right
  mesh.append_face(Face{ 21, 17, 18 });
  mesh.append_face(Face{ 21, 18, 22 });
  calc_normals(mesh, Object3D::ShadingMode::FLAT_SHADING);
}

void Cube::set_texture(const std::string& filename) {
  auto& mesh = m_meshes[0];
  mesh.texture() = std::make_shared<Texture2D>(filename);
  int cnt = 0;
  for (const auto& face : mesh.faces()) {
    assert(face.size == 3);
    for (int i = 0; i < face.size; ++i) 
    {
      Vertex& v = mesh.vertices()[face.data[i]];
      if (cnt % 2 == 0) {
        if (i == 0) {
          v.texture = glm::vec2();
        }
        else if (i == 1) {
          v.texture = glm::vec2(1.f, 0.f);
        }
        else {
          v.texture = glm::vec2(1.f, 1.f);
        }
      }
      else {
        if (i == 0) {
          v.texture = glm::vec2();
        }
        else if (i == 1) {
          v.texture = glm::vec2(1.f, 1.f);
        }
        else {
          v.texture = glm::vec2(0.f, 1.f);
        }
      }
    }
    cnt++;
  }
}
