#include "FileExplorerWindow.hpp"
#include "MenuBar.hpp"
#include "core/ModelLoader.hpp"
#include "core/WindowGLFW.hpp"
#include "core/SceneRenderer.hpp"
#include "core/Event.hpp"

#include <ImGuiFileDialog.h>
#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace fury
{
  FileExplorerWindow::FileExplorerWindow(SceneRenderer* scene) : UiComponent(scene)
  {
    scene->get_ui().get_component<MenuBar>("MenuBar")->on_open_file_explorer += new InstanceListener(this, &FileExplorerWindow::open_file_exporer);
  }

  void FileExplorerWindow::tick()
  {
    if (!is_visible())
    {
      return;
    }

    const bool is_scene_info_visible = m_scene->get_ui().get_component("SceneInfo")->is_visible();
    // file selection mode
    WindowGLFW* window = m_scene->get_window();
    if (!is_scene_info_visible)
    {
      m_scene->get_camera().freeze();
      m_scene->get_window()->get_input_handler(UserInputHandler::CURSOR_POSITION)->disable();
    }
    // show cursor
    glfwSetInputMode(window->gl_window(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // create file dialog
    IGFD::FileDialogConfig config;
    config.path = ".";

    // display in the middle of the screen
    ImGui::SetNextWindowPos(ImVec2(window->width() * 0.5f, window->height() * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    constexpr ImVec2 min_size(700, 300), max_size(1280, 650);
    auto& dlg = *ImGuiFileDialog::Instance();
    dlg.OpenDialog("FileExplorer", m_title, m_file_extensions.data(), config);
    if (dlg.Display("FileExplorer", ImGuiWindowFlags_NoCollapse, min_size, max_size))
    {
      if (ImGuiFileDialog::Instance()->IsOk())
      {
        std::string selected_file = dlg.GetFilePathName();
        m_callback(this, selected_file);
      }

      if (!is_scene_info_visible)
      {
        m_scene->get_camera().unfreeze();
        m_scene->get_window()->get_input_handler(UserInputHandler::CURSOR_POSITION)->enable();
        glfwSetInputMode(window->gl_window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      }
      ImGuiFileDialog::Instance()->Close();
      hide();
    }
  }

  void FileExplorerWindow::save_scene(const std::string& file)
  {
    m_scene->save(file);
  }

  void FileExplorerWindow::import_scene(const std::string& file)
  {
    m_scene->load(file);
  }

  void FileExplorerWindow::import_model_file(const std::string& file)
  {
    ModelLoader loader;
    std::optional<Object3D> m = loader.load(file, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_GenBoundingBoxes /*| aiProcess_GenSmoothNormals*/);
    if (m)
    {
      m_scene->get_drawables().push_back(std::make_unique<Object3D>(std::move(*m)));
      m_scene->on_new_object_added.notify(m_scene->get_drawables().back().get());
    }
  }

  void FileExplorerWindow::open_file_exporer(const OpenFileExplorerContext& ctx)
  {
    if (ctx.save_file)
    {
      m_callback = &FileExplorerWindow::save_scene;
      m_title = "Select where to save scene";
      m_file_extensions = ".bin";
    }
    else
    {
      if (ctx.is_scene_op)
      {
        m_callback = &FileExplorerWindow::import_scene;
        m_title = "Select scene to import";
        m_file_extensions = ".bin";
      }
      else
      {
        m_callback = &FileExplorerWindow::import_model_file;
        m_title = "Select file to import";
        m_file_extensions = ".obj";
      }
    }
    show();
  }
}
