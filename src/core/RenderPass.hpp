#pragma once

#include "ITickable.hpp"
#include "VertexArrayObject.hpp"
#include "VertexBufferObject.hpp"
#include "ElementBufferObject.hpp"
#include "SSBO.hpp"
#include "glm/glm.hpp"
#include "glad/glad.h"
#include <vector>
#include <map>
#include <set>

class Object3D;
class SceneRenderer;
struct ObjectChangeInfo;

class RenderPass : public ITickable
{
public:
  RenderPass(SceneRenderer* scene);
  virtual void update() = 0;
protected:
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
  GeometryPass(SceneRenderer* scene);
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
  VertexArrayObject m_vao_indices;
  VertexArrayObject m_vao_arrays;
  VertexBufferObject m_vbo_indices;
  VertexBufferObject m_vbo_arrays;
  ElementBufferObject m_ebo;
  std::map<const Object3D*, std::vector<MeshRenderOffsets>> m_render_offsets;
  std::vector<const Object3D*> m_objects_indices_rendering_mode;
  std::vector<const Object3D*> m_objects_arrays_rendering_mode;
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

class LinesPass : public RenderPass
{
public:
  LinesPass(SceneRenderer* scene);
  void update() override;
  void tick() override;
private:
  void handle_visible_bbox_toggle(Object3D* obj, bool is_visible);
private:
  VertexArrayObject m_vao;
  VertexBufferObject m_vbo;
  ElementBufferObject m_ebo;
  std::vector<const Object3D*> m_objects_with_visible_bboxes;
};
