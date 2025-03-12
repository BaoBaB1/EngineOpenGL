#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "core/Shader.hpp"
#include "core/GPUBuffers.hpp"
#include "core/Texture2D.hpp"
#include "core/ShaderStorage.hpp"
#include "ge/IDrawable.hpp"
#include "ge/Mesh.hpp"
#include "ge/BoundingBox.hpp"
#include "core/Material.hpp"
#include <unordered_map>
#include <optional>
#include <list>
#include <map>

class Object3D : public IDrawable
{
public:
  enum ShadingMode
  {
    NO_SHADING,
    FLAT_SHADING,
    SMOOTH_SHADING
  };
  struct RenderConfig
  {
    static RenderConfig& default() {
      static RenderConfig cfgdef;
      cfgdef.mode = GL_TRIANGLES;
      cfgdef.use_indices = true;
      return cfgdef;
    }
    int mode;
    bool use_indices;
  };
public:
  virtual void apply_shading(ShadingMode mode);
  virtual void set_color(const glm::vec4& color);
  glm::vec3 center() const;
  void set_texture(const std::shared_ptr<Texture2D>& tex, TextureType type, size_t mesh_idx);
  void set_shading_mode(ShadingMode mode) { m_shading_mode = mode; }
  void set_delta_time(float delta_time) { m_delta_time = delta_time; }
  void set_render_config(const RenderConfig& cfg) { m_render_config = cfg; }
  void rotate(float angle, const glm::vec3& axis);
  void scale(const glm::vec3& scale);
  void translate(const glm::vec3& translation);
  void add_mesh(Mesh&& mesh);
  void add_mesh(const Mesh& mesh);
  Mesh& emplace_mesh() { set_flag(GEOMETRY_MODIFIED, true); return m_meshes.emplace_back(); }
  std::vector<Vertex> normals_as_lines();
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
  ShadingMode shading_mode() const { return m_shading_mode; }
  const glm::mat4& model_matrix() const { return m_model_mat; }
  glm::mat4& model_matrix() { return m_model_mat; }
  const glm::vec4& color() const { return m_color; }
  size_t mesh_count() const { return m_meshes.size(); }
  Mesh& get_mesh(size_t idx) { return m_meshes[idx]; }
  const RenderConfig& get_render_config() const { return m_render_config; }
  const Mesh& get_mesh(size_t idx) const { return m_meshes[idx]; }
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
    RESET_CACHED_NORMALS = (1 << 5),
    HAS_MATERIAL = (1 << 6),
    GEOMETRY_MODIFIED = (1 << 7)
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
  Object3D();
  Object3D(const Object3D&) = default;
  Object3D(Object3D&&) = default;
  Object3D& operator=(const Object3D&) noexcept = default;
  Object3D& operator=(Object3D&&) noexcept = default;
  void calc_normals(Mesh&, ShadingMode);
  void set_flag(Flag flag, bool value) { value ? set_flag(flag) : clear_flag(flag); }
  void set_flag(Flag flag) { m_flags |= flag; }
  void clear_flag(Flag flag) { m_flags &= ~flag; }
  bool get_flag(Flag flag) const { return m_flags & flag; }
protected:
  std::vector<Mesh> m_meshes;
  mutable glm::vec3 m_center = glm::vec3(0.f);
  glm::mat4 m_model_mat = glm::mat4(1.f);
  glm::vec4 m_color = glm::vec4(1.f);
  float m_rotation_angle = 0.f;
  float m_delta_time = 0.f;
  glm::vec3 m_rotation_axis = glm::vec3(0.f);
  uint32_t m_flags = RESET_CACHED_NORMALS;
  ShadingMode m_shading_mode = ShadingMode::NO_SHADING;
  VertexFinder m_vertex_finder;
  BoundingBox m_bbox;             // bounding box which covers all meshes
  RenderConfig m_render_config = RenderConfig::default();
  std::map<ShadingMode, std::vector<Mesh>> m_cached_meshes;
};
