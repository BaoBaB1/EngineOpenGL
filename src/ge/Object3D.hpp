#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "core/Shader.hpp"
#include "core/GPUBuffers.hpp"
#include "core/Texture2D.hpp"
#include "core/ShaderStorage.hpp"
#include "ge/Entity.hpp"
#include "ge/Mesh.hpp"
#include "ge/BoundingBox.hpp"
#include "ge/IRayHittable.hpp"
#include "core/Material.hpp"
#include <unordered_map>
#include <optional>
#include <list>
#include <map>

class Object3D : public Entity, public IRayHittable
{
public:
  enum ShadingMode
  {
    NO_SHADING,
    FLAT_SHADING,
    SMOOTH_SHADING,
    LAST_ITEM
  };
  struct RenderConfig
  {
    int mode = GL_TRIANGLES;
    bool use_indices = true;
  };
public:
  template<typename T>
  static T* cast_to(Object3D* obj) { return static_cast<T*>(obj); }
  Object3D();
  Object3D(const std::string& name);
  virtual ~Object3D() = default;
  virtual void apply_shading(ShadingMode mode);
  virtual void set_color(const glm::vec4& color);
  virtual bool has_surface() const { return true; }
  std::optional<RayHit> hit(const Ray& ray) const override;
  glm::vec3 center() const;
  void set_texture(const std::shared_ptr<Texture2D>& tex, TextureType type, size_t mesh_idx);
  void set_shading_mode(ShadingMode mode) { m_shading_mode = mode; }
  void set_delta_time(float delta_time) { m_delta_time = delta_time; }
  void set_render_config(const RenderConfig& cfg) { m_render_config = cfg; }
  void set_meshes_data(const std::shared_ptr<std::vector<Mesh>>& meshes) { m_meshes = meshes; }
  void rotate(float angle, const glm::vec3& axis);
  void scale(const glm::vec3& scale);
  void translate(const glm::vec3& translation);
  void add_mesh(Mesh&& mesh);
  void add_mesh(const Mesh& mesh);
  Mesh& emplace_mesh() { set_flag(GEOMETRY_MODIFIED, true); return m_meshes->emplace_back(); }
  void calculate_bbox(bool force = false);
  float rotation_angle() const { return m_rotation_angle; }
  glm::vec3 rotation_axis() const { return m_rotation_axis; }
  glm::vec3 translation() const { return m_model_mat[3]; }
  glm::vec3 scale() const { return glm::vec3(glm::length(m_model_mat[0]), glm::length(m_model_mat[1]), glm::length(m_model_mat[2])); }
  void light_source(bool val) { set_flag(LIGHT_SOURCE, val); }
  void rotating(bool val) { set_flag(ROTATE_EACH_FRAME, val); }
  void visible_normals(bool val) { set_flag(VISIBLE_NORMALS, val); }
  void visible_bbox(bool val) { return set_flag(VISIBLE_BBOX, val); }
  void select(bool val) { return set_flag(IS_SELECTED, val); }
  void set_has_material(bool val) { return set_flag(HAS_MATERIAL, val); }
  bool is_normals_visible() const { return get_flag(VISIBLE_NORMALS); }
  bool is_rotating() const { return get_flag(ROTATE_EACH_FRAME); }
  bool is_light_source() const { return get_flag(LIGHT_SOURCE); }
  bool is_bbox_visible() const { return get_flag(VISIBLE_BBOX); }
  bool is_selected() const { return get_flag(IS_SELECTED); }
  bool has_material() const { return get_flag(HAS_MATERIAL); }
  std::shared_ptr<std::vector<Mesh>> get_meshes_data() { return m_meshes; }
  const std::shared_ptr<std::vector<Mesh>>& get_meshes_data() const { return m_meshes; }
  ShadingMode shading_mode() const { return m_shading_mode; }
  const glm::mat4& model_matrix() const { return m_model_mat; }
  glm::mat4& model_matrix() { return m_model_mat; }
  const glm::vec4& color() const { return m_color; }
  size_t mesh_count() const { return m_meshes->size(); }
  Mesh& get_mesh(size_t idx) { return (*m_meshes)[idx]; }
  const RenderConfig& get_render_config() const { return m_render_config; }
  const Mesh& get_mesh(size_t idx) const { return (*m_meshes)[idx]; }
  const BoundingBox& bbox() const { return m_bbox; }
  BoundingBox bbox() { return m_bbox; }
  friend class SceneRenderer;
  friend class Ui;
protected:
  enum Flag
  {
    ROTATE_EACH_FRAME = (1 << 0),
    VISIBLE_NORMALS = (1 << 1),
    LIGHT_SOURCE = (1 << 2),
    VISIBLE_BBOX = (1 << 3),
    IS_SELECTED = (1 << 4),
    HAS_MATERIAL = (1 << 5),
    GEOMETRY_MODIFIED = (1 << 6)
  };
  struct WrappedVertex {
    explicit WrappedVertex(const Vertex& vertex) {
      this->vertex = vertex;
    }
    bool operator==(const WrappedVertex& other) const { return other.vertex.position == this->vertex.position; }
    Vertex vertex;
  };
  struct VertexHasher {
    size_t operator()(const WrappedVertex& wrapped) const {
      std::hash<float> hasher;
      return hasher(wrapped.vertex.position.x) + hasher(wrapped.vertex.position.y) ^ hasher(wrapped.vertex.position.z);
    }
  };
  struct VertexFinder {
    using iter = std::unordered_map<WrappedVertex, GLuint, VertexHasher, std::equal_to<WrappedVertex>>::iterator;
    iter end() { return m_map_vert.end(); }
    iter find_vertex(const Vertex& v) {
      return m_map_vert.find(WrappedVertex(v));
    }
    void add_vertex(const Vertex& v, GLuint index) {
      m_map_vert.insert(std::make_pair(WrappedVertex(v), index));
    }
    std::unordered_map<WrappedVertex, GLuint, VertexHasher, std::equal_to<WrappedVertex>> m_map_vert; // vertex, index
  };
protected:
  void calc_normals(Mesh&, ShadingMode);
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
  glm::vec3 m_rotation_axis = glm::vec3(0.f);
  uint32_t m_flags = 0;
  ShadingMode m_shading_mode = ShadingMode::NO_SHADING;
  VertexFinder m_vertex_finder;
  BoundingBox m_bbox;             // bounding box which covers all meshes
  RenderConfig m_render_config;
  std::array<std::shared_ptr<std::vector<Mesh>>, ShadingMode::LAST_ITEM + 1> m_cached_meshes;
};
