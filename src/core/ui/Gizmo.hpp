#pragma once

#include "UiComponent.hpp"

namespace fury
{
	class SceneRenderer;

	class Gizmo : public UiComponent
	{
	public:
		Gizmo(SceneRenderer* scene);
		void tick(float) override;
	private:
		uint16_t m_gizmo_operation;
	};
}
