#include "FileSelectionWindow.hpp"
#include "core/ModelLoader.hpp"
#include "core/WindowGLFW.hpp"
#include "core/SceneRenderer.hpp"
#include "core/Event.hpp"

#include <ImGuiFileDialog.h>
#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace fury
{
  FileSelectionWindow::FileSelectionWindow(SceneRenderer* scene) : UiComponent(scene)
  {
    scene->get_ui().get_component<MenuBar>("MenuBar")->on_open_file_click += new InstanceListener(static_cast<UiComponent*>(this), &UiComponent::show);
  }

  void FileSelectionWindow::tick()
  {
    if (!is_visible())
    {
      return;
    }

    // file selection mode
    WindowGLFW* window = m_scene->get_window();
    m_scene->get_camera().freeze();
    // show cursor
    glfwSetInputMode(window->gl_window(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // create file dialog
    IGFD::FileDialogConfig config;
    config.path = ".";

    // display in the middle of the screen
    ImGui::SetNextWindowPos(ImVec2(window->width() * 0.5f, window->height() * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    constexpr ImVec2 min_size(700, 300), max_size(1280, 650);
    auto& dlg = *ImGuiFileDialog::Instance();
    dlg.OpenDialog("FileLoaderDialog", "Select file to load", ".obj", config);
    if (dlg.Display("FileLoaderDialog", ImGuiWindowFlags_NoCollapse, min_size, max_size))
    {
      if (ImGuiFileDialog::Instance()->IsOk())
      {
        std::string selected_file = dlg.GetFilePathName();
        ModelLoader loader;
        std::optional<Object3D> m = loader.load(selected_file, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_GenBoundingBoxes /*| aiProcess_GenSmoothNormals*/);
        if (m)
        {
          m_scene->get_drawables().push_back(std::make_unique<Object3D>(std::move(*m)));
          m_scene->on_new_object_added.notify(m_scene->get_drawables().back().get());
        }
      }

      m_scene->get_camera().unfreeze();
      if (!m_scene->get_ui().get_component("SceneInfo")->is_visible())
      {
        glfwSetInputMode(window->gl_window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      }
      ImGuiFileDialog::Instance()->Close();
      hide();
    }
  }
}
