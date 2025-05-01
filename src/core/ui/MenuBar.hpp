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
		const ImVec2& get_size() const { return m_size; }
	private:
		ImVec2 m_size;
	};
}
