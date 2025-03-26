#pragma once

#include "Event.hpp"
#include <array>

class SceneRenderer;
class WindowGLFW;
class Object3D;

class Ui
{
public:
  Ui() = default;
  Ui(SceneRenderer* scene, WindowGLFW* window);
  void init(SceneRenderer* scene, WindowGLFW* window);
  ~Ui();
  void render();
  Event<Object3D*, bool> on_visible_normals_button_pressed;
private:
  void render_object_properties(Object3D& drawable);
  void render_xyz_markers(float offset_from_left, float width);
private:
  std::array<bool, 16> m_imgui_statesb;
  uint16_t m_guizmo_operation;
  WindowGLFW* m_window = nullptr;
  SceneRenderer* m_scene = nullptr;
};