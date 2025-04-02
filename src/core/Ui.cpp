#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm/gtx/matrix_decompose.hpp"
#include "ImGuiFileDialog.h"
#include "ImGuizmo.h"

#include "Ui.hpp"
#include "SceneRenderer.hpp"
#include "WindowGLFW.hpp"
#include "ModelLoader.hpp"
#include "./ge/Object3D.hpp"
#include "input/KeyboardHandler.hpp"

#include <vector>
#include <string>

static std::string shading_mode_to_str(fury::Object3D::ShadingMode mode);
static bool once = true;
static glm::vec3 g_translation = {};
static glm::vec3 g_scale = {};

namespace fury
{
  Ui::Ui(SceneRenderer* scene, WindowGLFW* window)
  {
    init(scene, window);
  }

  void Ui::init(SceneRenderer* scene, WindowGLFW* window)
  {
    m_scene = scene;
    m_window = window;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    ImGui_ImplGlfw_InitForOpenGL(window->gl_window(), true); // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    if (once)
    {
      ImGui_ImplOpenGL3_Init();
      once = false;
    }
    m_guizmo_operation = ImGuizmo::OPERATION::TRANSLATE;
    m_imgui_statesb.fill(false);
  }

  Ui::~Ui()
  {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
  }

  void Ui::render()
  {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    bool& st_gui_opened = m_imgui_statesb[0];
    bool& st_polygone_mode = m_imgui_statesb[1];
    bool& st_file_selection = m_imgui_statesb[2];
    auto& scene = m_scene;

    // menu bar
    constexpr ImVec2 menubar_frame_padding = ImVec2(0, 10);
    constexpr ImVec2 menubar_item_spacing = ImVec2(29, 10);
    ImVec2 menubar_size;

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, menubar_frame_padding);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, menubar_item_spacing);
    if (ImGui::BeginMainMenuBar())
    {
      menubar_size = ImGui::GetWindowSize();
      if (ImGui::BeginMenu("File"))
      {
        if (ImGui::MenuItem("Load", nullptr, &st_file_selection))
        {
        }
        ImGui::EndMenu();
      }
      ImGui::EndMainMenuBar();
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();

    // file selection mode
    if (st_file_selection)
    {
      // disable all handlers
      m_window->notify_all(false);
      glfwSetInputMode(m_window->gl_window(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);

      // create file dialog
      IGFD::FileDialogConfig config;
      config.path = ".";

      // display in the middle of the screen
      ImGui::SetNextWindowPos(ImVec2(m_window->width() * 0.5f, m_window->height() * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
      ImVec2 min_size(700, 300), max_size(1280, 650);

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
            scene->m_drawables.push_back(std::move(*m));
            scene->on_new_object_added.notify(&scene->m_drawables.back());
          }
        }

        // close
        st_file_selection = false;
        if (!st_gui_opened)
        {
          m_window->notify_all(true);
          glfwSetInputMode(m_window->gl_window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        else
        {
          m_window->get_input_handler(UserInputHandler::MOUSE_INPUT)->notify(true);
        }
        ImGuiFileDialog::Instance()->Close();
      }
    }

    // open/closed configuration menu
    if (ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_GraveAccent))
    {
      st_gui_opened = !st_gui_opened;
      if (st_gui_opened)
      {
        // disable any camera movement
        m_scene->get_camera().freeze();
        glfwSetInputMode(m_window->gl_window(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
      }
      else
      {
        m_scene->get_camera().unfreeze();
        glfwSetInputMode(m_window->gl_window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      }
    }

    if (st_gui_opened)
    {
      // stick menu to the right side of window
      const ImVec2 sz = ImVec2(m_window->width(), m_window->height());
      constexpr float scale_factor = 0.2f;
      ImGui::SetNextWindowSize(ImVec2(sz.x * scale_factor, sz.y - menubar_size.y));
      ImGui::SetNextWindowPos(ImVec2(sz.x - sz.x * scale_factor, menubar_size.y));
      ImGui::Begin("Scene properties", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

      ImGui::Text("Scene objects");
      ImGui::PushItemWidth(-1);

      // Gizmo
      if (!scene->m_selected_objects.empty())
      {
        KeyboardHandler* kh = static_cast<KeyboardHandler*>(m_window->get_input_handler(UserInputHandler::KEYBOARD));
        // default mode is translation
        if (kh->get_keystate(KeyboardHandler::InputKey::T) == KeyboardHandler::KeyState::PRESSED)
        {
          m_guizmo_operation = ImGuizmo::OPERATION::TRANSLATE;
        }
        else if (kh->get_keystate(KeyboardHandler::InputKey::R) == KeyboardHandler::KeyState::PRESSED)
        {
          m_guizmo_operation = ImGuizmo::OPERATION::ROTATE;
        }
        else if (kh->get_keystate(KeyboardHandler::InputKey::S) == KeyboardHandler::KeyState::PRESSED)
        {
          m_guizmo_operation = ImGuizmo::OPERATION::SCALE;
        }

        ImGuizmo::BeginFrame();
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetRect(0, 0, m_window->width(), m_window->height());

        Object3D* obj = scene->m_selected_objects.back();
        Camera& cam = scene->m_camera;
        ImGuizmo::MODE gizmo_mode = ImGuizmo::MODE::LOCAL;

        glm::mat4 model_mat = obj->m_model_mat;
        ImGuizmo::Manipulate(glm::value_ptr(cam.view_matrix()), glm::value_ptr(scene->m_camera.get_projection_matrix()), static_cast<ImGuizmo::OPERATION>(m_guizmo_operation),
          gizmo_mode, glm::value_ptr(model_mat));
        if (ImGuizmo::IsUsing())
        {
          if (m_guizmo_operation == ImGuizmo::OPERATION::ROTATE)
          {
            glm::vec3 translation, scale, skew;
            glm::quat rotation;
            glm::vec4 perspective;
            glm::decompose(model_mat, scale, rotation, translation, skew, perspective);
            obj->m_rotation_angle = glm::degrees(glm::angle(rotation));
            //obj->m_rotation_axis = glm::axis(rotation);
          }
          obj->m_model_mat = model_mat;
          ObjectChangeInfo info;
          info.is_transformation_change = true;
          on_object_change.notify(obj, info);
        }
      }

      if (ImGui::BeginListBox("##ListBox"))
      {
        int idx = 0;
        for (auto& obj : scene->m_drawables)
        {
          bool selected = obj.is_selected();
          if (ImGui::Selectable((obj.get_name() + std::to_string(idx + 1)).c_str(), &selected))
          {
            scene->select_object(&obj, true);
          }
          ++idx;
        }
        ImGui::EndListBox();
      }
      ImGui::PopItemWidth();
      ImGui::Separator();
      ImGui::Dummy(ImVec2(0, 5));

      ImGui::SetNextItemOpen(true, ImGuiCond_::ImGuiCond_Once);
      if (ImGui::CollapsingHeader("General"))
      {
        if (ImGui::Checkbox("Don't fill polygons", &st_polygone_mode))
        {
          glGetIntegerv(GL_POLYGON_MODE, &scene->m_polygon_mode);
          if (scene->m_polygon_mode == GL_LINE)
            scene->m_polygon_mode = GL_FILL;
          else
            scene->m_polygon_mode = GL_LINE;
        }
      }

      if (scene->m_selected_objects.size())
      {
        render_object_properties(*scene->m_selected_objects.back());
      }

      ImGuiIO& io = ImGui::GetIO();
      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
      ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  }

  void Ui::render_object_properties(Object3D& drawable)
  {
    ImGui::SetNextItemOpen(true, ImGuiCond_::ImGuiCond_Once);
    const std::string& name = drawable.get_name();
    if (ImGui::TreeNode(name.c_str()))
    {
      // translation
      ImGui::Separator();
      ImGui::Text("Translation");
      ImGui::PushItemWidth(-1);
      const float item_width = ImGui::CalcItemWidth() - 5; // -5 to leave some offset from right
      ImGui::PopItemWidth();
      const float offset_from_left = ImGui::GetStyle().IndentSpacing;
      const float win_space_for_items_x = ImGui::GetWindowSize().x - offset_from_left;

      render_xyz_markers(offset_from_left, win_space_for_items_x);
      ImGui::PushItemWidth(-1);
      g_translation = drawable.translation();
      glm::vec3 old_translation = drawable.translation();
      if (ImGui::InputFloat3("##translationLabel", &g_translation.x))
      {
        drawable.translate(g_translation - old_translation);
      }
      ImGui::PopItemWidth();
      ImGui::Dummy(ImVec2(0.f, 5.f));
      ImGui::GetStyle().ItemSpacing.x = 3.f;
      ImGui::PushItemWidth(item_width / 3);

      if (ImGui::SliderFloat("##X", &g_translation.x, -100.0f, 100.0f))
        drawable.translate(glm::vec3(g_translation.x - old_translation.x, 0.f, 0.f));

      ImGui::SameLine();
      if (ImGui::SliderFloat("##Y", &g_translation.y, -100.0f, 100.0f))
        drawable.translate(glm::vec3(0.f, g_translation.y - old_translation.y, 0.f));

      ImGui::SameLine();
      if (ImGui::SliderFloat("##Z", &g_translation.z, -100.0f, 100.0f))
        drawable.translate(glm::vec3(0.f, 0.f, g_translation.z - old_translation.z));

      // scale
      ImGui::Separator();
      ImGui::Text("Scale");
      render_xyz_markers(offset_from_left, win_space_for_items_x);
      ImGui::PushItemWidth(-1);
      glm::vec3 scale = drawable.scale();
      g_scale = drawable.scale();
      if (ImGui::InputFloat3("##scaleLabel", &scale.x))
      {
        if (scale.x > 0 && scale.y > 0 && scale.z > 0)
          drawable.scale(scale);
      }
      ImGui::PopItemWidth();
      ImGui::Dummy(ImVec2(0, 5));
      ImGui::PushItemWidth(item_width / 3);

      if (ImGui::SliderFloat("##X2", &g_scale.x, 0.1f, 3.f))
        drawable.scale(g_scale);

      ImGui::SameLine();
      if (ImGui::SliderFloat("##Y2", &g_scale.y, 0.1f, 3.f))
        drawable.scale(g_scale);

      ImGui::SameLine();
      if (ImGui::SliderFloat("##Z2", &g_scale.z, 0.1f, 3.f))
        drawable.scale(g_scale);
      ImGui::PopItemWidth();

      // rotation
      ImGui::Separator();
      ImGui::Text("Rotation");
      bool rotating = drawable.is_rotating();
      float old_rot_angle = drawable.m_rotation_angle;

      // TODO: rework rotations
      if (ImGui::Checkbox("Rotate every frame", &rotating))
        drawable.rotating(rotating);

      if (ImGui::SliderFloat("Angle(deg)", &drawable.m_rotation_angle, -360.f, 360.f))
      {
        if (!drawable.is_rotating())
          drawable.rotate(drawable.m_rotation_angle - old_rot_angle, drawable.m_rotation_axis);
      }

      render_xyz_markers(offset_from_left, win_space_for_items_x);
      if (ImGui::SliderFloat("##X3", &drawable.m_rotation_axis.x, 0.f, 1.f))
      {
        if (!drawable.is_rotating())
          drawable.rotate(drawable.m_rotation_angle, drawable.m_rotation_axis);
      }

      ImGui::SameLine();
      if (ImGui::SliderFloat("##Y3", &drawable.m_rotation_axis.y, 0.f, 1.f))
      {
        if (!drawable.is_rotating())
          drawable.rotate(drawable.m_rotation_angle, drawable.m_rotation_axis);
      }

      ImGui::SameLine();
      if (ImGui::SliderFloat("##Z3", &drawable.m_rotation_axis.z, 0.f, 1.f))
      {
        if (!drawable.is_rotating())
          drawable.rotate(drawable.m_rotation_angle, drawable.m_rotation_axis);
      }

      // other properties
      ImGui::Separator();
      ImGui::PushItemWidth(-1);
      ImGui::Text("Color");
      if (ImGui::ColorEdit4("##Color", &drawable.m_color.x))
      {
        drawable.set_color(drawable.m_color);
        ObjectChangeInfo info;
        info.is_vertex_change = true;
        on_object_change.notify(&drawable, info);
      }
      ImGui::PopItemWidth();

      ImGui::Separator();
      ImGui::Text("Miscellaneous");
      bool is_bbox_visible = drawable.is_bbox_visible();
      if (ImGui::Checkbox("Show bounding box", &is_bbox_visible))
      {
        drawable.visible_bbox(is_bbox_visible);
        on_visible_bbox_button_pressed.notify(&drawable, is_bbox_visible);
      }

      // TODO: rewrite later as e.g. 2D Circle has surface but it won't be derived from Model class
      if (drawable.has_surface())
      {
        // calc size of next checkbox, if it fits width of properties window, put it on same line,
        // othewise, put on other line
        const auto next_line_cursor_pos = ImGui::GetCursorPos();
        ImGui::SameLine();
        const auto same_line_cursor_pos = ImGui::GetCursorPos();
        const float width_of_next_checkbox = ImGui::CalcTextSize("Visible normals").x
          + ImGui::GetFrameHeight() + ImGui::GetCurrentContext()->Style.ItemInnerSpacing.x;
        constexpr float x_offset = 25;
        if (same_line_cursor_pos.x + x_offset + width_of_next_checkbox > win_space_for_items_x)
        {
          ImGui::SetCursorPos(next_line_cursor_pos);
        }
        else {
          ImGui::Dummy(ImVec2(x_offset, 0));
          ImGui::SameLine();
        }

        bool is_normals_visible = drawable.is_normals_visible();
        if (ImGui::Checkbox("Visible normals", &is_normals_visible))
        {
          drawable.visible_normals(is_normals_visible);
          on_visible_normals_button_pressed.notify(&drawable, is_normals_visible);
        }

        if (!drawable.is_fixed_shading())
        {
          // shading modes
          std::vector<std::pair<Object3D::ShadingMode, std::string>> modes(3);
          for (int i = 0; i < 3; i++)
          {
            Object3D::ShadingMode mode = static_cast<Object3D::ShadingMode>(i);
            modes[i] = std::make_pair(mode, ::shading_mode_to_str(mode));
          }
          std::string current_mode = ::shading_mode_to_str(drawable.shading_mode());
          ImGui::SetNextItemWidth(win_space_for_items_x / 2);
          if (ImGui::BeginCombo("Shading mode", current_mode.c_str()))
          {
            for (int i = 0; i < 3; i++)
            {
              bool selected = (current_mode == modes[i].second);
              if (ImGui::Selectable(modes[i].second.c_str(), &selected))
              {
                drawable.apply_shading(modes[i].first);
                ObjectChangeInfo info;
                info.is_shading_mode_change = true;
                on_object_change.notify(&drawable, info);
              }
              if (selected)
                ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
          }
          ImGui::Separator();
        }
      }
      ImGui::TreePop();
    }
  }

  void Ui::render_xyz_markers(float offset_from_left, float width)
  {
    constexpr const char* XYZ[] = { "X", "Y", "Z" };
    for (int i = 0; i < 3; i++)
    {
      // center labels over input fields
      ImGui::SetCursorPosX(offset_from_left + (width / 3 * i) + width / 6);
      ImGui::Text(XYZ[i]);
      if (i < 2)
        ImGui::SameLine();
    }
  }
}

static std::string shading_mode_to_str(fury::Object3D::ShadingMode mode)
{
  switch (mode)
  {
  case fury::Object3D::ShadingMode::SMOOTH_SHADING:
    return "Smooth shading";
  case fury::Object3D::ShadingMode::FLAT_SHADING:
    return "Flat shading";
  case fury::Object3D::ShadingMode::NO_SHADING:
    return "No shading";
  default:
    return "Unknown";
  }
}
