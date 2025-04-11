#pragma once

#include "UiComponent.hpp"
#include <string>
#include <functional>

namespace fury
{
	class SceneRenderer;
	struct OpenFileExplorerContext;

	class FileExplorerWindow : public UiComponent
	{
	public:
		FileExplorerWindow(SceneRenderer* scene);
		void tick() override;
	private:
		void import_model_file(const std::string& file);
		void save_scene(const std::string& file);
		void import_scene(const std::string& file);
		void open_file_exporer(const OpenFileExplorerContext&);
	private:
		std::function<void(FileExplorerWindow*, const std::string& file)> m_callback;
		std::string m_title;
		std::string m_file_extensions;
	};
}
