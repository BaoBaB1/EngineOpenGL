#pragma once

#include "UiComponent.hpp"
#include "core/Event.hpp"
#include <string>
#include <functional>

namespace fury
{
	class SceneRenderer;
	struct OpenFileExplorerContext;

	class FileExplorer : public UiComponent
	{
	public:
		FileExplorer(SceneRenderer* scene);
		void open(const OpenFileExplorerContext& ctx, std::function<void(const std::string&)> callback);
		void tick() override;
	private:
		std::string m_title;
		std::string m_file_extensions;
		std::function<void(const std::string&)> m_callback;
	};
}
