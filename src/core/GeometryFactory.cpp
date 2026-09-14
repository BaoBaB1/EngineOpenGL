#include "GeometryFactory.hpp"
#include "ShadingProcessor.hpp"
#include "utils/Utils.hpp"

namespace
{
  using namespace fury;
  void create_cube_mesh(Mesh& mesh);
  void create_pyramid_mesh(Mesh& mesh);
  void create_icosahedron_mesh(Mesh& mesh);
  void create_bezier_curve_mesh(Mesh& mesh, const ecs::BezierCurveParametersComponent& params);
  void subdivide_icosahedron_mesh(Mesh& mesh, uint32_t subdlevel);
} // namespace

namespace fury
{
  Mesh GeometryFactory::create_cube(const ecs::CubeParametersComponent& params, ShadingMode shading_mode)
  {
    Mesh m;
    create_cube_mesh(m);
    ShadingProcessor::apply_shading(m, shading_mode);
    return m;
  }

  Mesh GeometryFactory::create_pyramid(const ecs::PyramidParametersComponent& params, ShadingMode shading_mode)
  {
    Mesh m;
    create_pyramid_mesh(m);
    ShadingProcessor::apply_shading(m, shading_mode);
    return m;
  }

  Mesh GeometryFactory::create_icosahedron(const ecs::IcosahedronParametersComponent& params, ShadingMode shading_mode)
  {
    Mesh m;
    create_icosahedron_mesh(m);
    subdivide_icosahedron_mesh(m, params.subdivisiton_depth);
    if (params.project_points_on_sphere)
    {
      for (Vertex& v : m.get_vertices())
      {
        v.position = glm::normalize(v.position);
      }
    }
    ShadingProcessor::apply_shading(m, shading_mode);
    return m;
  }

  Mesh GeometryFactory::create_bezier_curve(const ecs::BezierCurveParametersComponent& params)
  {
    Mesh m;
    create_bezier_curve_mesh(m, params);
    return m;
  }
} // namespace fury

namespace
{
  void create_cube_mesh(Mesh& mesh)
  {
    mesh.get_vertices().clear();
    mesh.get_faces().clear();
    mesh.get_vertices().reserve(24);
    mesh.get_faces().reserve(12);
    for (int i = 0; i < 3; ++i)
    {
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

    // uv mapping
    for (int cnt = 0; const auto& face : mesh.get_faces())
    {
      Vertex& v1 = mesh.get_vertices()[face[0]];
      Vertex& v2 = mesh.get_vertices()[face[1]];
      Vertex& v3 = mesh.get_vertices()[face[2]];
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

  void create_pyramid_mesh(Mesh& mesh)
  {
    auto& vertices = mesh.get_vertices();
    auto& faces = mesh.get_faces();
    vertices.clear();
    faces.clear();
    vertices.reserve(5);
    faces.reserve(6);
    vertices.emplace_back(-0.5f, -0.5f, -0.5f);
    vertices.emplace_back(0.5f, -0.5f, -0.5f);
    vertices.emplace_back(0.5f, -0.5f, 0.5f);
    vertices.emplace_back(-0.5f, -0.5f, 0.5f);
    vertices.emplace_back(0.f, 0.5f, 0.f);
    faces.emplace_back(Face({ 0, 1, 2 }));
    faces.emplace_back(Face({ 0, 2, 3 }));
    faces.emplace_back(Face({ 0, 4, 1 }));
    faces.emplace_back(Face({ 1, 4, 2 }));
    faces.emplace_back(Face({ 2, 4, 3 }));
    faces.emplace_back(Face({ 3, 4, 0 }));
  }

  void create_icosahedron_mesh(Mesh& mesh)
  {
    mesh.get_vertices().clear();
    mesh.get_faces().clear();
    mesh.get_vertices().reserve(12);
    mesh.get_faces().reserve(20);
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

  void create_bezier_curve_mesh(Mesh& mesh, const ecs::BezierCurveParametersComponent& params)
  {
    constexpr float step = 0.005f;
    std::vector<Vertex>& vertices = mesh.get_vertices();
    vertices.clear();
    vertices.reserve(static_cast<size_t>(1. / step));

    if (params.curve_type == BezierCurveType::Quadratic)
    {
      // B(t) = (1-t)^2 * P0 + 2t(1-t) * P1 + t^2 * P2 , where
      // P0 - start point
      // P1 - control point
      // P2 - end point
      // 0 <= t <= 1
      for (float t = 0.0; t <= 1.0; t += step)
      {
        Vertex res = ((1 - t) * (1 - t) * params.start_point) + (2 * t * (1 - t) * params.control_point1) +
                     (t * t * params.end_point);
        vertices.push_back(res);
      }
    }
    else
    {
      // B(t) = ( (1-t)^3 * P0 ) + ( 3(1-t)^2 * t * P1 ) + ( 3(1-t) * t^2 * P2 ) + (t^3 * P3), where
      // P0 - start point
      // P1 - control point
      // P2 - control point
      // P3 - end point
      // 0 <= t <= 1
      const glm::vec3& P0 = params.start_point;
      const glm::vec3& P1 = params.control_point1;
      const glm::vec3& P2 = params.control_point2;
      const glm::vec3& P3 = params.end_point;
      for (float t = 0.0; t <= 1.0; t += step)
      {
        Vertex res = (std::pow((1 - t), 3.f) * P0) + (3 * std::pow((1 - t), 2.f) * t * P1) +
                     (3 * (1 - t) * t * t * P2) + (std::pow(t, 3.f) * P3);
        vertices.push_back(res);
      }
    }
  }

  void subdivide_icosahedron_mesh(Mesh& mesh, uint32_t subdlevel)
  {
    auto allocate_memory_before_subdivision = [](Mesh& mesh, uint32_t subdlevel)
    {
      const size_t new_face_count = mesh.get_faces().size() * std::pow<size_t>(4, subdlevel);
      const size_t new_vert_count = new_face_count * 3;
      mesh.get_vertices().reserve(new_vert_count);
      mesh.get_faces().reserve(new_face_count);
      Logger::debug("Preallocating memory for {} icosahedron points and {} faces.", new_vert_count, new_face_count);
    };

    auto subdivide_triangles = [](Mesh& mesh, uint32_t subdlevel, const Vertex& a, const Vertex& b, const Vertex& c)
    {
      // Wrap to call lambda recursively
      auto do_work =
          [](const auto& self, Mesh& mesh, uint32_t subdlevel, const Vertex& a, const Vertex& b, const Vertex& c)
      {
        if (subdlevel == 0)
        {
          GLuint ind = static_cast<GLuint>(mesh.append_vertex(a));
          GLuint ind2 = static_cast<GLuint>(mesh.append_vertex(b));
          GLuint ind3 = static_cast<GLuint>(mesh.append_vertex(c));
          mesh.append_face(Face({ ind, ind2, ind3 }));
          return;
        }
        Vertex ab((a.position + b.position) / 2.f);
        Vertex bc((b.position + c.position) / 2.f);
        Vertex ac((a.position + c.position) / 2.f);
        // ab.color = bc.color = ac.color = m_color;
        //  ORDER IS IMPORTANT !!!
        self(self, mesh, subdlevel - 1, a, ab, ac);
        self(self, mesh, subdlevel - 1, b, bc, ab);
        self(self, mesh, subdlevel - 1, c, ac, bc);
        self(self, mesh, subdlevel - 1, ab, bc, ac);
      };
      do_work(do_work, mesh, subdlevel, a, b, c);
    };

    allocate_memory_before_subdivision(mesh, subdlevel);
    std::vector<Face> ico_level0_faces = std::move(mesh.get_faces());
    std::vector<Vertex> ico_level0_vertices = std::move(mesh.get_vertices());
    for (size_t i = 0; i < ico_level0_faces.size(); ++i)
    {
      GLuint index = ico_level0_faces[i][0];
      GLuint index2 = ico_level0_faces[i][1];
      GLuint index3 = ico_level0_faces[i][2];
      subdivide_triangles(mesh, subdlevel, ico_level0_vertices[index], ico_level0_vertices[index2],
                          ico_level0_vertices[index3]);
    }
  }
} // namespace
