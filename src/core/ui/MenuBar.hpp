#pragma once

#include "UiComponent.hpp"
#include "core/Event.hpp"
#include <string>

namespace fury
{
	class SceneRenderer;

	struct OpenFileExplorerContext
	{
		OpenFileExplorerContext(bool save_file_, bool is_scene_op_) 
			: save_file(save_file_), is_scene_op(is_scene_op_) {}
		bool save_file;
		bool is_scene_op;
	};

	class MenuBar : public UiComponent
	{
	public:
		MenuBar(SceneRenderer* scene);
		void tick() override;
		Event<const OpenFileExplorerContext&> on_open_file_explorer;
		//Event<const std::string&> on_add_object_click;
		const ImVec2& get_size() const { return m_size; }
	private:
		ImVec2 m_size;
	};
}
