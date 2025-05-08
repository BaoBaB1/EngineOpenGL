#pragma once

#include "ge/Mesh.hpp"
#include "ge/BoundingBox.hpp"
#include "ge/IRayHittable.hpp"
#include "ge/ShadingProcessor.hpp"
#include "core/ISerializable.hpp"
#include "core/ObjectsRegistry.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <optional>

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

  class Object3D : public IRayHittable, public ISerializable
  {
  public:
    using ShadingMode = ShadingProcessor::ShadingMode;
    struct RenderConfig
    {
      int mode = GL_TRIANGLES;
      bool use_indices = true;
    };
  public:
    template<typename T>
    static T* cast_to(Object3D* obj) { return static_cast<T*>(obj); }
    Object3D() = default;
    Object3D(const std::string& name);
    virtual uint32_t get_type() const { return ObjectsRegistry::get_id<Object3D>(); }
    void read(std::ifstream&) override;
    void write(std::ofstream&) const override;
    void set_color(const glm::vec4& color);
    ObjectGeometryMetadata get_geometry_metadata() const;
    std::optional<RayHit> hit(const Ray& ray) const override;
    glm::vec3 center() const;
    void update();
    void apply_shading(ShadingMode mode);
    void set_shading_mode(ShadingMode mode) { m_shading_mode = mode; }
    void set_delta_time(float delta_time) { m_delta_time = delta_time; }
    void set_render_config(const RenderConfig& cfg) { m_render_config = cfg; }
    void set_meshes_data(const std::shared_ptr<std::vector<Mesh>>& meshes) { m_meshes = meshes; }
    void rotate(float angle, const glm::vec3& axis);
    void set_rotation_angle(float angle) { m_rotation_angle = angle; }
    void set_name(const std::string& name) { m_name = name; }
    void scale(const glm::vec3& scale);
    void translate(const glm::vec3& translation);
    void add_mesh(Mesh&& mesh);
    void add_mesh(const Mesh& mesh);
    Mesh& emplace_mesh() { return m_meshes->emplace_back(); }
    void calculate_bbox(bool force = false);
    float rotation_angle() const { return m_rotation_angle; }
    glm::vec3 rotation_axis() const { return m_rotation_axis; }
    glm::vec3 translation() const { return m_model_mat[3]; }
    glm::vec3 scale() const { return glm::vec3(glm::length(m_model_mat[0]), glm::length(m_model_mat[1]), glm::length(m_model_mat[2])); }
    void light_source(bool val) { set_flag(LIGHT_SOURCE, val); }
    void rotating(bool val) { set_flag(ROTATE_EACH_FRAME, val); }
    void visible_normals(bool val) { set_flag(VISIBLE_NORMALS, val); }
    void visible_bbox(bool val) { set_flag(VISIBLE_BBOX, val); }
    void select(bool val) { set_flag(IS_SELECTED, val); }
    void set_is_fixed_shading(bool val) { set_flag(IS_FIXED_SHADING, val); }
    bool is_normals_visible() const { return get_flag(VISIBLE_NORMALS); }
    bool is_rotating() const { return get_flag(ROTATE_EACH_FRAME); }
    bool is_light_source() const { return get_flag(LIGHT_SOURCE); }
    bool is_bbox_visible() const { return get_flag(VISIBLE_BBOX); }
    bool is_selected() const { return get_flag(IS_SELECTED); }
    bool is_fixed_shading() const { return get_flag(IS_FIXED_SHADING); }
    bool has_surface() const { return get_flag(HAS_SURFACE); }
    std::shared_ptr<std::vector<Mesh>> get_meshes_data() { return m_meshes; }
    const std::shared_ptr<std::vector<Mesh>>& get_meshes_data() const { return m_meshes; }
    ShadingMode shading_mode() const { return m_shading_mode; }
    const glm::mat4& model_matrix() const { return m_model_mat; }
    glm::mat4& model_matrix() { return m_model_mat; }
    const glm::vec4& color() const { return m_color; }
    size_t mesh_count() const { return m_meshes->size(); }
    Mesh& get_mesh(size_t idx) { return (*m_meshes)[idx]; }
    std::vector<Mesh>& get_meshes() { return *m_meshes; }
    const std::vector<Mesh>& get_meshes() const { return *m_meshes; }
    const RenderConfig& get_render_config() const { return m_render_config; }
    const Mesh& get_mesh(size_t idx) const { return (*m_meshes)[idx]; }
    const BoundingBox& bbox() const { return m_bbox; }
    BoundingBox& bbox() { return m_bbox; }
    const std::string& get_name() const { return m_name; }
  protected:
    enum Flag
    {
      ROTATE_EACH_FRAME = (1 << 0),
      VISIBLE_NORMALS = (1 << 1),
      LIGHT_SOURCE = (1 << 2),
      VISIBLE_BBOX = (1 << 3),
      IS_SELECTED = (1 << 4),
      IS_FIXED_SHADING = (1 << 5),
      HAS_SURFACE = (1 << 6)
    };
  protected:
    void set_flag(Flag flag, bool value) { value ? set_flag(flag) : clear_flag(flag); }
    void set_flag(Flag flag) { m_flags |= flag; }
    void clear_flag(Flag flag) { m_flags &= ~flag; }
    bool get_flag(Flag flag) const { return m_flags & flag; }
  protected:
    std::shared_ptr<std::vector<Mesh>> m_meshes = std::make_shared<std::vector<Mesh>>();
    mutable glm::vec3 m_center = glm::vec3(0.f);
    glm::mat4 m_model_mat = glm::mat4(1.f);
    glm::vec4 m_color = glm::vec4(1.f);
    float m_rotation_angle = 0.f;
    float m_delta_time = 0.f;
    bool m_need_update = false;
    glm::vec3 m_rotation_axis = glm::vec3(0.f);
    uint32_t m_flags = 0;
    ShadingMode m_shading_mode = ShadingMode::NO_SHADING;
    BoundingBox m_bbox;             // bounding box which covers all meshes
    RenderConfig m_render_config;
    std::array<std::shared_ptr<std::vector<Mesh>>, ShadingMode::LAST_ITEM + 1> m_cached_meshes;
    std::string m_name;
  };
}
