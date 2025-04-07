#pragma once

#include "UiComponent.hpp"
#include "core/Event.hpp"
#include <string>

namespace fury
{
	class SceneRenderer;

	class MenuBar : public UiComponent
	{
	public:
		MenuBar(SceneRenderer* scene);
		void tick() override;
		Event<> on_open_file_click;
		//Event<const std::string&> on_add_object_click;
		const ImVec2& get_size() const { return m_size; }
	private:
		ImVec2 m_size;
	};
}
