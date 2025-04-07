#pragma once

#include "UiComponent.hpp"

namespace fury
{
	class SceneRenderer;

	class FileSelectionWindow : public UiComponent
	{
	public:
		FileSelectionWindow(SceneRenderer* scene);
		void tick() override;
	};
}
