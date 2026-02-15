#pragma once

#include "UiComponent.hpp"
#include "core/Event.hpp"

namespace fury
{
	struct ObjectChangeInfo;
	class SceneRenderer;
	class Object3D;

	class Gizmo : public UiComponent
	{
	public:
		Gizmo(SceneRenderer* scene);
		void tick(float) override;
		Event<Object3D*, const ObjectChangeInfo&> on_object_change;
	private:
		uint16_t m_gizmo_operation;
	};
}
