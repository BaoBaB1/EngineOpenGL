#include "Object3D.hpp"
#include "core/Logger.hpp"

namespace fury
{
  ObjectGeometryMetadata Object3D::get_geometry_metadata() const
  {
    ObjectGeometryMetadata res;
    res.meshes_data.reserve(m_meshes->size());
    for (const auto& mesh : *m_meshes)
    {
      MeshGeometryMetadata& mesh_data = res.meshes_data.emplace_back();
      res.vert_count_total += mesh.vertices().size();
      mesh_data.vert_count = mesh.vertices().size();
      if (m_render_config.use_indices)
      {
        assert(m_render_config.mode == GL_TRIANGLES);
        res.face_count_total += mesh.faces().size();
        mesh_data.face_count = mesh.faces().size();
        mesh_data.vdata = mesh.vertices().data();
        mesh_data.idx_data = mesh.faces_as_indices().data();
      }
    }
    return res;
  }

  void Object3D::add_mesh(Mesh&& mesh)
  {
    m_meshes->push_back(std::move(mesh));
  }

  void Object3D::add_mesh(const Mesh& mesh)
  {
    m_meshes->push_back(mesh);
  }

  void Object3D::calculate_bbox(bool force)
  {
    if (!force && !m_bbox.is_empty())
    {
      return;
    }
    glm::vec3 min(INFINITY), max(-INFINITY);
    for (const auto& mesh : *m_meshes)
    {
      for (const Vertex& v : mesh.vertices())
      {
        min = glm::min(min, v.position);
        max = glm::max(max, v.position);
      }
    }
    m_bbox.init(min, max);
  }

  void Object3D::set_color(const glm::vec4& color)
  {
    m_color = color;
    auto apply_color = [](std::vector<Mesh>& meshes, const glm::vec4& color)
      {
        for (auto& mesh : meshes)
        {
          for (auto& vertex : mesh.vertices())
          {
            vertex.color = color;
          }
        }
      };

    // set color of current mesh
    apply_color(*m_meshes, color);

    // set color of cached meshes
    for (auto& cached_meshes : m_cached_meshes)
    {
      if (cached_meshes)
      {
        apply_color(*cached_meshes, color);
      }
    }
  }

  glm::vec3 Object3D::center() const
  {
    if (!m_need_update)
    {
      return m_center;
    }
    assert(m_meshes->size() > 0);
    glm::vec3 min(INFINITY), max(-INFINITY);
    for (const auto& mesh : *m_meshes)
    {
      for (const auto& v : mesh.vertices())
      {
        min = glm::min(min, v.position);
        max = glm::max(max, v.position);
      }
    }
    m_center = (min + max) * 0.5f;
    return m_center;
  }

  void Object3D::update()
  {
    m_need_update = true;
    center();
    calculate_bbox(true);
    m_need_update = false;
  }

  void Object3D::apply_shading(ShadingProcessor::ShadingMode mode)
  {
    if (get_flag(IS_FIXED_SHADING) || !has_surface())
      return;
    if (mode != m_shading_mode)
    {
      // if current mesh is not cached
      if (!m_cached_meshes[m_shading_mode])
      {
        m_cached_meshes[m_shading_mode] = m_meshes;
      }
      // if mesh with requested shading mode is already in cache
      if (auto mesh_ptr_from_cache = m_cached_meshes[mode])
      {
        m_meshes = mesh_ptr_from_cache;
        m_shading_mode = mode;
        return;
      }
      // copy data
      auto current_meshes_tmp = m_meshes;
      m_meshes = std::make_shared<std::vector<Mesh>>();
      for (const auto& mesh : *current_meshes_tmp)
      {
        m_meshes->emplace_back(mesh.vertices(), mesh.faces());
      }
      ShadingProcessor::apply_shading(*m_meshes, mode);
      m_shading_mode = mode;
    }
  }
}
