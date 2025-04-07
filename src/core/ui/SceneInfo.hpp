#pragma once

#include "UiComponent.hpp"
#include "core/Event.hpp"
#include "core/input/KeyboardHandler.hpp"
#include <glm/glm.hpp>

namespace fury
{
	class SceneRenderer;
	class Object3D;
	struct ObjectChangeInfo;

	class SceneInfo : public UiComponent
	{
	public:
		SceneInfo(SceneRenderer* scene);
		void tick() override;
		Event<Object3D*, bool> on_visible_normals_button_pressed;
		Event<Object3D*, bool> on_visible_bbox_button_pressed;
		Event<Object3D*, const ObjectChangeInfo&> on_object_change;
		Event<int> on_polygon_mode_change;
	private:
		void render_object_properties(Object3D& drawable);
		void render_xyz_markers(float offset_from_left, float width);
		void handle_key_press(KeyboardHandler::InputKey key, KeyboardHandler::KeyState state);
	private:
		uint16_t m_guizmo_operation;
		bool m_fill_polygons = true;
		glm::vec3 m_obj_translation;
		glm::vec3 m_obj_scale;
		glm::vec3 m_obj_rotation_axis;
		float m_obj_rotation_angle = 0;
		glm::vec4 m_obj_color;
	};
}