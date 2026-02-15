#include "FileExplorer.hpp"
#include "core/WindowGLFW.hpp"
#include "core/SceneRenderer.hpp"
#include "core/AssetManager.hpp"

#include <ImGuiFileDialog.h>
#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace fury
{
  FileExplorer::FileExplorer(SceneRenderer* scene) : UiComponent(scene)
  {
  }

  void FileExplorer::tick(float)
  {
    if (!m_is_visible)
    {
      return;
    }

    WindowGLFW* window = m_scene->get_window();
    // create file dialog
    IGFD::FileDialogConfig config;
    config.path = AssetManager::get_assets_folder().string();

    // display in the middle of the screen
    ImGui::SetNextWindowPos(ImVec2(window->width() * 0.5f, window->height() * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    constexpr ImVec2 min_size(700, 300), max_size(1280, 650);
    auto& dlg = *ImGuiFileDialog::Instance();
    dlg.OpenDialog("FileExplorer", m_title, m_file_extensions.data(), config);
    if (dlg.Display("FileExplorer", ImGuiWindowFlags_NoCollapse, min_size, max_size))
    {
      if (ImGuiFileDialog::Instance()->IsOk())
      {
        m_callback(dlg.GetFilePathName());
      }
      ImGuiFileDialog::Instance()->Close();
      hide();
    }
  }

  void FileExplorer::open(const OpenFileExplorerContext& ctx, std::function<void(const std::string&)> callback)
  {
    m_callback = callback;
    if (ctx.save_scene)
    {
      m_title = "Select where to save scene";
      m_file_extensions = ".bin";
    }
    else if (ctx.load_scene)
    {
      m_title = "Select scene to import";
      m_file_extensions = ".bin";
    }
    else if (ctx.import_asset)
    {
      m_title = "Select file to import";
      m_file_extensions = ".obj";
    }
    else if (ctx.select_texture)
    {
      m_title = "Select texture for object";
      m_file_extensions = ".jpg,.jpeg,.png";
    }
    show();
  }
}
