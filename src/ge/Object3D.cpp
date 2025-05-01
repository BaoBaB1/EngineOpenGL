#include "Object3D.hpp"
#include "core/Logger.hpp"
#include "utils/Utils.hpp"
#include "core/AssetManager.hpp"

namespace fury
{
  Object3D::Object3D() : Entity("Object")
  {
  }

  Object3D::Object3D(const std::string& name) : Entity(name)
  {
  }

  void Object3D::read(std::ifstream& ifs)
  {
    m_meshes->clear();
    for (auto& m : m_cached_meshes)
    {
      if (m)
        m->clear();
    }

    // read general info
    ifs.read(reinterpret_cast<char*>(&m_center), sizeof(glm::vec3));
    ifs.read(reinterpret_cast<char*>(&m_model_mat), sizeof(glm::mat4));
    ifs.read(reinterpret_cast<char*>(&m_color), sizeof(glm::vec4));
    ifs.read(reinterpret_cast<char*>(&m_rotation_axis), sizeof(glm::vec3));
    ifs.read(reinterpret_cast<char*>(&m_rotation_angle), sizeof(float));
    ifs.read(reinterpret_cast<char*>(&m_need_update), sizeof(bool));
    ifs.read(reinterpret_cast<char*>(&m_flags), sizeof(decltype(m_flags)));
    ifs.read(reinterpret_cast<char*>(&m_shading_mode), sizeof(decltype(m_shading_mode)));
    ifs.read(reinterpret_cast<char*>(&m_render_config), sizeof(RenderConfig));
    ifs.read(reinterpret_cast<char*>(&m_bbox.min()), sizeof(glm::vec3));
    ifs.read(reinterpret_cast<char*>(&m_bbox.max()), sizeof(glm::vec3));

    // read geometry data
    size_t meshes_count = 0;
    ifs.read(reinterpret_cast<char*>(&meshes_count), sizeof(size_t));
    m_meshes->reserve(meshes_count);
    for (size_t i = 0; i < meshes_count; i++)
    {
      Mesh& mesh = m_meshes->emplace_back();

      // read vertices
      size_t vcount = 0;
      ifs.read(reinterpret_cast<char*>(&vcount), sizeof(size_t));
      mesh.vertices().resize(vcount);
      ifs.read(reinterpret_cast<char*>(mesh.vertices().data()), vcount * sizeof(Vertex));

      // read faces
      size_t fcount = 0;
      ifs.read(reinterpret_cast<char*>(&fcount), sizeof(size_t));
      mesh.faces().resize(fcount);
      ifs.read(reinterpret_cast<char*>(mesh.faces().data()), fcount * sizeof(Face));

      // material
      ifs.read(reinterpret_cast<char*>(&mesh.material()), sizeof(Material));
      
      // textures
      size_t tex_count = 0;
      ifs.read(reinterpret_cast<char*>(&tex_count), sizeof(size_t));
      for (size_t j = 0; j < tex_count; j++)
      {
        TextureType ttype;
        size_t filename_len;
        ifs.read(reinterpret_cast<char*>(&ttype), sizeof(TextureType));
        ifs.read(reinterpret_cast<char*>(&filename_len), sizeof(size_t));
        // read path relative to assets folder
        std::string file(filename_len, ' ');
        ifs.read(reinterpret_cast<char*>(file.data()), filename_len);
        auto absolute_path = AssetManager::get_from_relative(file);
        if (!absolute_path)
        {
          Logger::error("Failed to read object's texture with relative path {}.", file);
          continue;
        }
        mesh.set_texture(std::make_shared<Texture2D>(absolute_path.value()), ttype);
      }

      // mesh bbox
      ifs.read(reinterpret_cast<char*>(&mesh.bbox().min()), sizeof(glm::vec3));
      ifs.read(reinterpret_cast<char*>(&mesh.bbox().max()), sizeof(glm::vec3));
    }
  }

  void Object3D::write(std::ofstream& ofs) const
  {
    // write general info
    int32_t type = get_type();
    ofs.write(reinterpret_cast<const char*>(&type), sizeof(decltype(type)));
    ofs.write(reinterpret_cast<const char*>(&m_center), sizeof(glm::vec3));
    ofs.write(reinterpret_cast<const char*>(&m_model_mat), sizeof(glm::mat4));
    ofs.write(reinterpret_cast<const char*>(&m_color), sizeof(glm::vec4));
    ofs.write(reinterpret_cast<const char*>(&m_rotation_axis), sizeof(glm::vec3));
    ofs.write(reinterpret_cast<const char*>(&m_rotation_angle), sizeof(float));
    ofs.write(reinterpret_cast<const char*>(&m_need_update), sizeof(bool));
    ofs.write(reinterpret_cast<const char*>(&m_flags), sizeof(decltype(m_flags)));
    ofs.write(reinterpret_cast<const char*>(&m_shading_mode), sizeof(decltype(m_shading_mode)));
    ofs.write(reinterpret_cast<const char*>(&m_render_config), sizeof(RenderConfig));
    ofs.write(reinterpret_cast<const char*>(&m_bbox.min()), sizeof(glm::vec3));
    ofs.write(reinterpret_cast<const char*>(&m_bbox.max()), sizeof(glm::vec3));

    // write geometry data
    const size_t meshes_count = m_meshes->size();
    ofs.write(reinterpret_cast<const char*>(&meshes_count), sizeof(size_t));
    for (const Mesh& mesh : *m_meshes)
    {
      const size_t vcount = mesh.vertices().size();
      ofs.write(reinterpret_cast<const char*>(&vcount), sizeof(size_t));
      ofs.write(reinterpret_cast<const char*>(mesh.vertices().data()), sizeof(Vertex) * mesh.vertices().size());
      const size_t fcount = mesh.faces().size();
      ofs.write(reinterpret_cast<const char*>(&fcount), sizeof(size_t));
      // here we write triangle faces only. now quad faces are not used at all, but there is FaceN<4>.
      ofs.write(reinterpret_cast<const char*>(mesh.faces().data()), sizeof(Face) * mesh.faces().size());
      ofs.write(reinterpret_cast<const char*>(&mesh.material()), sizeof(Material));
      const auto textures = mesh.get_present_textures();
      const size_t tex_count = textures.size();
      ofs.write(reinterpret_cast<const char*>(&tex_count), sizeof(size_t));
      size_t tex_count_offset = ofs.tellp();
      size_t validated_textures = 0;
      for (const auto& [ttype, tex] : textures)
      {
        // type + file
        // TODO: add option for embedding texture directly into binary instead for saving just path
        // at this point texture must be in assets folder
        auto opt_tex = AssetManager::get_from_absolute(tex->get_file());
        if (!opt_tex)
        {
          Logger::error("Could not get relative texture path from absolute path {}.", tex->get_file());
          continue;
        }
        validated_textures++;
        const std::string relative_path = opt_tex.value();
        const size_t path_len = relative_path.size();
        ofs.write(reinterpret_cast<const char*>(&ttype), sizeof(TextureType));
        ofs.write(reinterpret_cast<const char*>(&path_len), sizeof(size_t));
        ofs.write(reinterpret_cast<const char*>(relative_path.data()), path_len);
      }
      // this is not tested
      if (validated_textures != tex_count)
      {
        size_t current_pos = ofs.tellp();
        ofs.seekp(tex_count_offset, std::ios_base::beg);
        ofs.write(reinterpret_cast<const char*>(validated_textures), sizeof(size_t));
        ofs.seekp(current_pos, std::ios_base::beg);
      }
      ofs.write(reinterpret_cast<const char*>(&mesh.bbox().min()), sizeof(glm::vec3));
      ofs.write(reinterpret_cast<const char*>(&mesh.bbox().max()), sizeof(glm::vec3));
    }
  }

  void Object3D::rotate(float angle, const glm::vec3& axis)
  {
    if (axis == glm::vec3())
      return;
    constexpr float rotation_speed = 10.f;
    m_rotation_angle = angle;
    m_rotation_axis = axis;
    m_model_mat = glm::rotate(m_model_mat, glm::radians(angle * m_delta_time * rotation_speed), glm::normalize(axis));
  }

  void Object3D::scale(const glm::vec3& scale)
  {
    // get rid of current scale factor (https://gamedev.stackexchange.com/questions/119702/fastest-way-to-neutralize-scale-in-the-transform-matrix)
    for (int i = 0; i < 3; i++)
      m_model_mat[i] = glm::normalize(m_model_mat[i]);
    m_model_mat = glm::scale(m_model_mat, scale);
  }

  void Object3D::translate(const glm::vec3& translation)
  {
    m_model_mat = glm::translate(m_model_mat, translation);
  }

  std::optional<RayHit> Object3D::hit(const Ray& ray) const
  {
    if (m_bbox.is_empty())
    {
      const_cast<Object3D*>(this)->calculate_bbox();
    }
    std::optional<RayHit> rhit;
    if (ray.intersect_aabb(m_bbox))
    {
      if (m_render_config.mode == GL_TRIANGLES)
      {
        if (m_render_config.use_indices)
        {
          for (const auto& mesh : *m_meshes)
          {
            for (const auto& face : mesh.faces())
            {
              if (auto hit = ray.intersect_triangle(
                mesh.get_vertex(face[0]).position, mesh.get_vertex(face[1]).position, mesh.get_vertex(face[2]).position)
                )
              {
                // find closest hit
                if (!rhit || rhit->distance > hit->distance)
                  rhit = hit;
              }
            }
          }
        }
        else
        {
          for (const auto& mesh : *m_meshes)
          {
            const auto& vertices = mesh.vertices();
            for (size_t i = 0; i < vertices.size(); i += 3)
            {
              if (auto hit = ray.intersect_triangle(
                mesh.get_vertex(i).position, mesh.get_vertex(i + 1).position, mesh.get_vertex(i + 2).position)
                )
              {
                // find closest hit
                if (!rhit || rhit->distance > hit->distance)
                  rhit = hit;
              }
            }
          }
        }
      }
      else
      {
        Logger::info("Bounding box is intersected, but could not test if object is actually hit. Primitives are not triangles");
      }
    }
    return rhit;
  }

  ObjectGeometryMetadata Object3D::get_geometry_metadata() const
  {
    ObjectGeometryMetadata res;
    for (const auto& mesh : *m_meshes)
    {
      MeshGeometryMetadata& mesh_data = res.meshes_data.emplace_back();
      res.vert_count_total += mesh.vertices().size();
      mesh_data.vert_count = mesh.vertices().size();
      if (m_render_config.use_indices)
      {
        assert(m_render_config.mode == GL_TRIANGLES);
        res.idx_count_total += mesh.faces().size() * 3;
        mesh_data.idx_count = mesh.faces().size() * 3;
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
    if (get_flag(IS_FIXED_SHADING))
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
