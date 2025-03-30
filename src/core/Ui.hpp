#pragma once

#include "Event.hpp"
#include "utils/Macro.hpp"
#include <array>

class SceneRenderer;
class WindowGLFW;
class Object3D;

struct ObjectChangeInfo
{
  //bool is_geometry_change = false;
  bool is_vertex_change = false;
  bool is_transformation_change = false;
};

class Ui
{
public:
  Ui() = default;
  Ui(SceneRenderer* scene, WindowGLFW* window);
  OnlyMovable(Ui)
  ~Ui();
  void init(SceneRenderer* scene, WindowGLFW* window);
  void render();
  Event<Object3D*, bool> on_visible_normals_button_pressed;
  Event<Object3D*, bool> on_visible_bbox_button_pressed;
  Event<Object3D*, const ObjectChangeInfo&> on_object_change;
private:
  void render_object_properties(Object3D& drawable);
  void render_xyz_markers(float offset_from_left, float width);
private:
  std::array<bool, 16> m_imgui_statesb;
  uint16_t m_guizmo_operation;
  WindowGLFW* m_window = nullptr;
  SceneRenderer* m_scene = nullptr;
};