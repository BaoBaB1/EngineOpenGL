#pragma once

#include "core/ITickable.hpp"
#include "imgui.h"

namespace fury
{
	class SceneRenderer;

	class UiComponent : public ITickable
	{
	public:
		UiComponent(SceneRenderer* scene);
		void show() { m_is_visible = true; }
		void hide() { m_is_visible = false; }
		bool is_visible() const { return m_is_visible; }
	protected:
		bool m_is_visible = false;
		SceneRenderer* m_scene = nullptr;
	};
}
