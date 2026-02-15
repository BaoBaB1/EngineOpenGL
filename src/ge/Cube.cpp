#include "Cube.hpp"

namespace fury
{
  Cube::Cube()
  {
    auto& mesh = emplace_mesh();
    mesh.vertices().reserve(24);
    mesh.faces().reserve(12);
    for (int i = 0; i < 3; ++i) {
      mesh.append_vertex(Vertex(-0.5f, -0.5f, 0.5f));
      mesh.append_vertex(Vertex(0.5f, -0.5f, 0.5f));
      mesh.append_vertex(Vertex(0.5f, 0.5f, 0.5f));
      mesh.append_vertex(Vertex(-0.5f, 0.5f, 0.5f));
      mesh.append_vertex(Vertex(-0.5f, -0.5f, -0.5f));
      mesh.append_vertex(Vertex(0.5f, -0.5f, -0.5f));
      mesh.append_vertex(Vertex(0.5f, 0.5f, -0.5f));
      mesh.append_vertex(Vertex(-0.5f, 0.5f, -0.5f));
    }
    // front
    mesh.append_face(Face({ 0, 1, 2 }));
    mesh.append_face(Face({ 0, 2, 3 }));
    // back
    mesh.append_face(Face({ 5, 4, 7 }));
    mesh.append_face(Face({ 5, 7, 6 }));
    // bottom
    mesh.append_face(Face({ 12, 13, 9 }));
    mesh.append_face(Face({ 12, 9, 8 }));
    // top
    mesh.append_face(Face({ 11, 10, 14 }));
    mesh.append_face(Face({ 11, 14, 15 }));
    // left
    mesh.append_face(Face({ 20, 16, 19 }));
    mesh.append_face(Face({ 20, 19, 23 }));
    // right
    mesh.append_face(Face({ 17, 21, 22 }));
    mesh.append_face(Face({ 17, 22, 18 }));

    // by default we create cube with flat shading
    set_shading_mode(ShadingMode::FLAT_SHADING);
    ShadingProcessor::calc_normals(mesh, ShadingMode::FLAT_SHADING);

    // uv mapping
    int cnt = 0;
    for (const auto& face : mesh.faces())
    {
      Vertex& v1 = mesh.vertices()[face.data[0]];
      Vertex& v2 = mesh.vertices()[face.data[1]];
      Vertex& v3 = mesh.vertices()[face.data[2]];
      if (cnt % 2 == 0)
      {
        v1.uv = glm::vec2();
        v2.uv = glm::vec2(1.f, 0.f);
        v3.uv = glm::vec2(1.f, 1.f);
      }
      else
      {
        v1.uv = glm::vec2();
        v2.uv = glm::vec2(1.f, 1.f);
        v3.uv = glm::vec2(0.f, 1.f);
      }
      cnt++;
    }
  }
}
