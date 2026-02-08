#pragma once

#include "ge/Mesh.hpp"
#include "ge/BoundingBox.hpp"
#include "ge/ShadingProcessor.hpp"
#include "core/Macros.hpp"
#include "core/Entity.hpp"
#include <glm/glm.hpp>

namespace fury
{
  struct MeshGeometryMetadata
  {
    size_t vert_count = 0;
    size_t face_count = 0;
    const Vertex* vdata = nullptr;
    const GLuint* idx_data = nullptr;
  };

  struct ObjectGeometryMetadata
  {
    size_t vert_count_total = 0;
    size_t face_count_total = 0;
    std::vector<MeshGeometryMetadata> meshes_data;
  };

  class Object3D : public Entity
  {
  public:
    using ShadingMode = ShadingProcessor::ShadingMode;
    struct RenderConfig
    {
      int mode = GL_TRIANGLES;
      bool use_indices = true;
    };
  public:
    FURY_REGISTER_DERIVED_CLASS(Object3D, Entity)
    template<typename T>
    static T* cast_to(Object3D* obj) { return static_cast<T*>(obj); }
    template<typename T>
    T* cast_to() { return static_cast<T*>(this); }
    void set_color(const glm::vec4& color);
    ObjectGeometryMetadata get_geometry_metadata() const;
    glm::vec3 center() const;
    void update();
    void apply_shading(ShadingMode mode);
    void set_shading_mode(ShadingMode mode) { m_shading_mode = mode; }
    void set_meshes_data(const std::shared_ptr<std::vector<Mesh>>& meshes) { m_meshes = meshes; }
    void add_mesh(Mesh&& mesh);
    void add_mesh(const Mesh& mesh);
    Mesh& emplace_mesh() { return m_meshes->emplace_back(); }
    void calculate_bbox(bool force = false);
    void visible_normals(bool val) { set_flag(VISIBLE_NORMALS, val); }
    void visible_bbox(bool val) { set_flag(VISIBLE_BBOX, val); }
    void select(bool val) { set_flag(IS_SELECTED, val); }
    void set_is_fixed_shading(bool val) { set_flag(IS_FIXED_SHADING, val); }
    bool is_normals_visible() const { return get_flag(VISIBLE_NORMALS); }
    bool is_bbox_visible() const { return get_flag(VISIBLE_BBOX); }
    bool is_selected() const { return get_flag(IS_SELECTED); }
    bool is_fixed_shading() const { return get_flag(IS_FIXED_SHADING); }
    bool has_surface() const { return get_flag(HAS_SURFACE); }
    std::shared_ptr<std::vector<Mesh>> get_meshes_data() { return m_meshes; }
    const std::shared_ptr<std::vector<Mesh>>& get_meshes_data() const { return m_meshes; }
    ShadingMode shading_mode() const { return m_shading_mode; }
    const glm::vec4& color() const { return m_color; }
    size_t mesh_count() const { return m_meshes->size(); }
    Mesh& get_mesh(size_t idx) { return (*m_meshes)[idx]; }
    std::vector<Mesh>& get_meshes() { return *m_meshes; }
    const std::vector<Mesh>& get_meshes() const { return *m_meshes; }
    const Mesh& get_mesh(size_t idx) const { return (*m_meshes)[idx]; }
    const BoundingBox& get_bbox() const { return m_bbox; }
    BoundingBox& get_bbox() { return m_bbox; }
    FURY_PROPERTY_REF(name, std::string, m_name)
    FURY_PROPERTY_REF(render_config, RenderConfig, m_render_config)
    FURY_DECLARE_SERIALIZABLE_FIELDS(
      FURY_SERIALIZABLE_FIELD(1, &Object3D::m_flags),
      FURY_SERIALIZABLE_FIELD(2, &Object3D::m_color),
      FURY_SERIALIZABLE_FIELD(3, &Object3D::m_meshes),
      FURY_SERIALIZABLE_FIELD(4, &Object3D::m_shading_mode),
      FURY_SERIALIZABLE_FIELD(5, &Object3D::m_bbox),
      FURY_SERIALIZABLE_FIELD(6, &Object3D::m_render_config),
      FURY_SERIALIZABLE_FIELD(7, &Object3D::m_center)
    )
  protected:
    enum Flag
    {
      VISIBLE_NORMALS = (1 << 0),
      VISIBLE_BBOX = (1 << 1),
      IS_SELECTED = (1 << 2),
      IS_FIXED_SHADING = (1 << 3),
      HAS_SURFACE = (1 << 4)
    };
  protected:
    void set_flag(Flag flag, bool value) { value ? set_flag(flag) : clear_flag(flag); }
    void set_flag(Flag flag) { m_flags |= flag; }
    void clear_flag(Flag flag) { m_flags &= ~flag; }
    bool get_flag(Flag flag) const { return m_flags & flag; }
  protected:
    std::shared_ptr<std::vector<Mesh>> m_meshes = std::make_shared<std::vector<Mesh>>();
    mutable glm::vec3 m_center = glm::vec3(0.f);
    glm::vec4 m_color = glm::vec4(1.f);
    bool m_need_update = false;
    uint32_t m_flags = HAS_SURFACE;
    ShadingMode m_shading_mode = ShadingMode::NO_SHADING;
    BoundingBox m_bbox;             // bounding box which covers all meshes
    RenderConfig m_render_config;
    std::array<std::shared_ptr<std::vector<Mesh>>, ShadingMode::LAST_ITEM + 1> m_cached_meshes;
  };
}
