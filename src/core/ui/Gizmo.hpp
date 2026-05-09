#pragma once

#include "UiComponent.hpp"

namespace fury
{
	class Scene;

	class Gizmo : public UiComponent
	{
	public:
		Gizmo(Scene* scene);
		void tick(float) override;
	private:
		uint16_t m_gizmo_operation;
	};
}
