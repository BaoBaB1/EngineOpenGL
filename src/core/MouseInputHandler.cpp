#include "MouseInputHandler.hpp"
#include "MainWindow.hpp"
#include "SceneRenderer.hpp"

MouseInputHandler::MouseInputHandler(MainWindow* window) : UserInputHandler(window, HandlerType::MOUSE_INPUT)
{
  auto lb_click_callback = [](GLFWwindow* window, int button, int action, int mods)
    {
      glfwSetWindowUserPointer(window, m_ptrs[HandlerType::MOUSE_INPUT]);
      static_cast<MouseInputHandler*>(glfwGetWindowUserPointer(window))->left_btn_click_callback(window, button, action, mods);
    };

  auto window_size_change_callback = [](GLFWwindow* window, int width, int height)
    {
      glfwSetWindowUserPointer(window, m_ptrs[HandlerType::MOUSE_INPUT]);
      static_cast<MouseInputHandler*>(glfwGetWindowUserPointer(window))->window_size_change_callback(window, width, height);
    };

  glfwSetWindowSizeCallback(m_window->gl_window(), window_size_change_callback);
  glfwSetMouseButtonCallback(m_window->gl_window(), lb_click_callback);
}

void MouseInputHandler::left_btn_click_callback(GLFWwindow* window, int button, int action, int mods)
{
  if (!disabled())
  {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
      double xd, yd;
      glfwGetCursorPos(window, &xd, &yd);
      auto& scene = SceneRenderer::instance();
      const int x = static_cast<int>(xd);
      const int y = scene.m_window->height() - static_cast<int>(yd);
      const auto& picking_fbo = scene.m_fbos["picking"];
      picking_fbo.bind();
      glBindTexture(GL_TEXTURE_2D, picking_fbo.texture()->id());
      uint32_t id = 0;
      glReadBuffer(GL_COLOR_ATTACHMENT0);
      glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, &id);
      if (id != 0)
      {
        std::cout << "Pixel " << x << ',' << y << " object id = " << id << '\n';
        scene.select_object(id - 1, false);
      }
      glReadBuffer(0);
      picking_fbo.unbind();
    }
  }
}

void MouseInputHandler::window_size_change_callback(GLFWwindow* window, int width, int height)
{
  // to avoid crash when window is completely minimized
  if (width == 0 || height == 0)
    return;
  auto& s = SceneRenderer::instance();
  s.m_window->set_width(width);
  s.m_window->set_height(height);
  s.m_projection_mat = glm::mat4(1.f);
  s.m_projection_mat = glm::perspective(glm::radians(45.f), (float)width / height, 0.1f, 100.f);
  for (auto& [name, fbo] : s.m_fbos)
  {
    fbo.bind();
    // resize texture and render buffer
    fbo.attach_texture(width, height, fbo.texture()->internal_fmt(), fbo.texture()->format(), fbo.texture()->pixel_data_type());
    fbo.attach_renderbuffer(width, height, fbo.rb_internal_format(), fbo.rb_attachment());
    fbo.unbind();
  }
  glViewport(0, 0, width, height);
}
