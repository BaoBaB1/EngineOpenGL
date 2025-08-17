#pragma once

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

namespace fury
{
  class Object3D;
  class SceneRenderer;
  class Polyline;
  struct ObjectChangeInfo;
  class ItemSelectionWheel;
  struct SelectionWheelSlot;

  class RenderPass : public ITickable
  {
  public:
    RenderPass(SceneRenderer* scene);
    virtual void update() = 0;
  protected:
    SceneRenderer* m_scene = nullptr;
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
    GeometryPass(SceneRenderer* scene, int shadow_map_texture);
    void update() override;
    void tick() override;
  private:
    void allocate_memory_for_buffers();
    void split_objects();
    void render_scene();
    void render_selected_objects();
    void on_new_scene_object(Object3D* obj);
    void handle_object_change(Object3D* obj, const ObjectChangeInfo& info);
  private:
    // share all buffers data with shadow pass to avoid same data duplication
    friend class ShadowsPass;
    VertexArrayObject m_vao_indices;
    VertexArrayObject m_vao_arrays;
    VertexBufferObject m_vbo_indices;
    VertexBufferObject m_vbo_arrays;
    ElementBufferObject m_ebo;
    std::map<const Object3D*, std::vector<MeshRenderOffsets>> m_render_offsets;
    std::vector<const Object3D*> m_objects_indices_rendering_mode;
    std::vector<const Object3D*> m_objects_arrays_rendering_mode;
    int m_shadow_map_texture;
  };

  class ShadowsPass : public RenderPass
  {
  public:
    ShadowsPass(SceneRenderer* scene, GeometryPass* gp);
    void update() override;
    void tick() override;
  private:
    GeometryPass* m_gp;
  };

  // points + geometry shader
  class NormalsPass : public RenderPass
  {
    struct ObjectRenderOffsets
    {
      // vertex count for current model
      size_t vcount = 0;
      // offset in vbo (in vertices count)
      size_t vbo_offset_vtx_count = 0;
      // object index in m_model_matrices vector
      size_t internal_idx = 0;
    };
  public:
    NormalsPass(SceneRenderer* scene);
    void update() override;
    void tick() override;
  private:
    void handle_visible_normals_toggle(Object3D* obj, bool is_visible);
    void handle_object_change(Object3D* obj, const ObjectChangeInfo& info);
  private:
    VertexArrayObject m_vao;
    VertexBufferObject m_vbo;
    SSBO m_model_matrices_ssbo;
    std::set<const Object3D*> m_objects_with_visible_normals;
    std::vector<glm::mat4> m_model_matrices;
    std::vector<GLsizei> m_voffsets;
    std::vector<GLsizei> m_vcounts;
    std::map<const Object3D*, ObjectRenderOffsets> m_object_offsets;
  };

  class BoundingBoxPass : public RenderPass
  {
  public:
    BoundingBoxPass(SceneRenderer* scene);
    void update() override;
    void tick() override;
  private:
    void handle_visible_bbox_toggle(Object3D* obj, bool is_visible);
    void handle_scene_visible_bbox_toggle(bool is_visible);
    void handle_object_change(Object3D*, const ObjectChangeInfo&);
  private:
    VertexArrayObject m_vao;
    VertexBufferObject m_vbo;
    VertexBufferObject m_vbo_instance;
    ElementBufferObject m_ebo;
    bool m_is_scene_bbox_visible = false;
    uint32_t m_instances = 0;
  };

  class SelectionWheelPass : public RenderPass
  {
  public:
    SelectionWheelPass(SceneRenderer* scene, ItemSelectionWheel* wheel);
    void update() override;
    void tick() override;
  private:
    ItemSelectionWheel* m_wheel = nullptr;
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
    InfiniteGridPass(SceneRenderer* scene);
    void update() override {}
    void tick() override;
  private:
    VertexArrayObject m_vao;
  };

  class DebugPass : public RenderPass
  {
  public:
    DebugPass(SceneRenderer* scene);
    void update() override;
    void tick() override;
    void add_poly(const Polyline& poly);
    void clear();
  private:
    VertexArrayObject m_vao;
    VertexBufferObject m_vbo;
    std::vector<Polyline> m_polys;
  };

}
