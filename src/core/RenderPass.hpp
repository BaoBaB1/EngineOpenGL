#pragma once

#include "Common.hpp"
#include "ITickable.hpp"
#include "opengl/VertexArrayObject.hpp"
#include "opengl/VertexBufferObject.hpp"
#include "opengl/ElementBufferObject.hpp"
#include "opengl/SSBO.hpp"
#include "glm/glm.hpp"
#include "glad/glad.h"
#include <vector>
#include <map>
#include <set>
#include <optional>

namespace fury
{
  class Scene;
  class ItemSelectionWheel;
  struct SelectionWheelSlot;
  class BoundingBox;
  struct Frustum;

  class RenderPass : public ITickable
  {
  public:
    RenderPass() = default;
    RenderPass(Scene* scene);
    virtual void init(Scene* scene);
    virtual void update() = 0;
  protected:
    Scene* m_scene = nullptr;
  };

  class GeometryPass : public RenderPass
  {
    struct MeshRenderOffsets
    {
      size_t vbo_indices_offset = 0;
      // index offset (not in bytes)
      size_t vbo_arrays_offset = 0;
      size_t ebo_offset = 0;
      size_t basev = 0;
    };
  public:
    GeometryPass();
    void set_shadow_map_texture(int shadow_map_texture) { m_shadow_map_texture = shadow_map_texture; }
    void update() override;
    void update_mesh_data(EntityId entity);
    void tick(float) override;
    uint32_t get_num_culled_objects() const { return m_num_culled_objects; }
  private:
    void allocate_memory_for_buffers();
    void split_objects();
    void render_entities(const std::vector<EntityId>& entities, const Frustum& frustum);
    void render_selected_objects();
  private:
    // share all buffers data with shadow pass to avoid same data duplication
    friend class ShadowsPass;
    VertexArrayObject m_vao_indices;
    VertexArrayObject m_vao_arrays;
    VertexBufferObject m_vbo_indices;
    VertexBufferObject m_vbo_arrays;
    ElementBufferObject m_ebo;
    std::map<EntityId, std::vector<MeshRenderOffsets>> m_render_offsets;
    std::vector<EntityId> m_objects_indices_rendering_mode;
    std::vector<EntityId> m_objects_arrays_rendering_mode;
    std::vector<EntityId> m_objects_without_lighting;
    uint32_t m_shadow_map_texture = 0;
    uint32_t m_num_culled_objects = 0;
  };

  class ShadowsPass : public RenderPass
  {
  public:
    ShadowsPass() = default;
    void set_geometry_pass_ref(GeometryPass* gp) { m_gp = gp; }
    void update() override;
    void tick(float) override;
  private:
    GeometryPass* m_gp = nullptr;
  };

  class SelectionWheelPass : public RenderPass
  {
  public:
    SelectionWheelPass() = default;
    void init(Scene* scene) override;
    void update() override;
    void tick(float) override;
  private:
    // fop all wheel geometry
    VertexArrayObject m_vao;
    VertexBufferObject m_vbo;
    // for icons
    VertexArrayObject m_vao_icons;
    VertexBufferObject m_vbo_icons;
    glm::mat4 m_ortho = glm::mat4(1.f);
    std::vector<const SelectionWheelSlot*> m_slots_with_icons;
  };

  class InfiniteGridPass : public RenderPass
  {
  public:
    InfiniteGridPass() = default;
    InfiniteGridPass(Scene* scene);
    void update() override {}
    void tick(float) override;
  private:
    VertexArrayObject m_vao;
  };

  class DebugPass : public RenderPass
  {
  public:
    DebugPass() = default;
    void init(Scene* scene) override;
    void update() override;
    void update_normals();
    void tick(float);
    void add_line(const glm::vec3& a, const glm::vec3& b, const glm::vec4& color = glm::vec4(1, 1, 1, 1));
    void clear();
    void clear_lines();
  private:
    struct DebugVertex
    {
      glm::vec3 pos;
      glm::vec4 color;
    };
    struct AABBBoundsGPU
    {
      glm::vec4 minp;
      glm::vec4 maxp;
    };
  private:
    // Lines
    VertexArrayObject m_lines_vao;
    VertexBufferObject m_lines_vbo;
    std::vector<DebugVertex> m_lines_vertices;
    // AABBs
    VertexArrayObject m_bbox_vao;
    SSBO m_bbox_min_max_points_ssbo;
    // Normals
    VertexArrayObject m_normals_vao;
    VertexBufferObject m_normals_vbo;
    std::vector<GLsizei> m_normals_voffsets;
    std::vector<GLsizei> m_normals_vcounts;
    SSBO m_normals_transform_indices_ssbo;
  };

} // namespace fury
