#pragma once

#include "UiComponent.hpp"

namespace fury
{
	class SceneRenderer;

	struct OpenFileExplorerContext
	{
		bool save_scene = false;
		bool load_scene = false;
		bool import_asset = false;
		bool select_texture = false;
	};

	class MenuBar : public UiComponent
	{
	public:
		MenuBar(SceneRenderer* scene);
		void tick() override;
		const float get_height() const { return m_height; }
	private:
		float m_height = 0;
	};
}
