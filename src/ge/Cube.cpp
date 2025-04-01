#include "Cube.hpp"

Cube::Cube() : Object3D("Cube")
{
  auto& mesh = emplace_mesh();
  mesh.vertices().reserve(24);
  mesh.faces().reserve(12);
  for (int i = 0; i < 3; ++i) {
    mesh.append_vertex(Vertex(-0.5f, -0.5f, -0.5f));
    mesh.append_vertex(Vertex(0.5f, -0.5f, -0.5f));
    mesh.append_vertex(Vertex(0.5f, 0.5f, -0.5f));
    mesh.append_vertex(Vertex(-0.5f, 0.5f, -0.5f)); 
    mesh.append_vertex(Vertex(-0.5f, -0.5f, 0.5f));
    mesh.append_vertex(Vertex(0.5f, -0.5f, 0.5f));
    mesh.append_vertex(Vertex(0.5f, 0.5f, 0.5f));
    mesh.append_vertex(Vertex(-0.5f, 0.5f, 0.5f));
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

  // by default we create cube with flat shading
  set_shading_mode(ShadingMode::FLAT_SHADING);
  ShadingProcessor::calc_normals(mesh, ShadingMode::FLAT_SHADING);

  // uv mapping
  int cnt = 0;
  for (const auto& face : mesh.faces())
  {
    for (int i = 0; i < face.size; ++i)
    {
      Vertex& v = mesh.vertices()[face.data[i]];
      if (cnt % 2 == 0)
      {
        if (i == 0)
        {
          v.texture = glm::vec2();
        }
        else if (i == 1)
        {
          v.texture = glm::vec2(1.f, 0.f);
        }
        else
        {
          v.texture = glm::vec2(1.f, 1.f);
        }
      }
      else
      {
        if (i == 0)
        {
          v.texture = glm::vec2();
        }
        else if (i == 1)
        {
          v.texture = glm::vec2(1.f, 1.f);
        }
        else
        {
          v.texture = glm::vec2(0.f, 1.f);
        }
      }
    }
    cnt++;
  }
}
