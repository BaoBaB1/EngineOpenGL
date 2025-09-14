#pragma once

#include "UiComponent.hpp"
#include <glm/glm.hpp>

namespace fury
{
	class SceneRenderer;
	class Object3D;
	struct ObjectChangeInfo;
	class MenuBar;
	class Light;

	class SceneInfo : public UiComponent
	{
	public:
		SceneInfo(SceneRenderer* scene, MenuBar* menubar);
		bool is_grid_visible() const { return m_show_grid; }
		void tick() override;
		Event<Object3D*, bool> on_visible_normals_button_pressed;
		Event<Object3D*, bool> on_visible_bbox_button_pressed;
		Event<Object3D*, const ObjectChangeInfo&> on_object_change;
		Event<const Light*, bool> light_visibility_toggle;
		Event<int> on_polygon_mode_change;
		Event<bool> on_show_scene_bbox;
		Event<bool> msaa_button_click;
	private:
		void render_object_properties(Object3D& drawable);
		void render_xyz_markers(float offset_from_left, float width, float spacing);
		void render_fps_locks();
	private:
		glm::vec4 m_obj_color;
		glm::vec3 m_obj_translation;
		glm::vec3 m_obj_scale;
		MenuBar* m_menubar = nullptr;
		int m_fps_cap = 0;
		uint16_t m_guizmo_operation;
		bool m_fill_polygons = true;
		bool m_show_scene_bbox = false;
		bool m_use_msaa = true;
		bool m_use_vsync = true;
		bool m_show_grid = false;
	};
}