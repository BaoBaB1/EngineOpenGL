#include "CursorPositionHandler.hpp"
#include "SceneRenderer.hpp"
#include "MainWindow.hpp"

extern int ignore_frames;

CursorPositionHandler::CursorPositionHandler(MainWindow* window) : UserInputHandler(window, HandlerType::CURSOR_POSITION)
{
  auto callback = [](GLFWwindow* window, double xpos, double ypos)
    {
      // this must point to MainWindow instance
      void* old_ptr = glfwGetWindowUserPointer(window);
      assert(old_ptr);
      glfwSetWindowUserPointer(window, m_ptrs[HandlerType::CURSOR_POSITION]);
      static_cast<CursorPositionHandler*>(glfwGetWindowUserPointer(window))->callback(xpos, ypos);
      glfwSetWindowUserPointer(window, old_ptr);
    };
  glfwSetCursorPosCallback(m_window->gl_window(), callback);
  glfwGetCursorPos(m_window->gl_window(), &m_cur_pos[0], &m_cur_pos[1]);
  m_prev_pos[0] = m_prev_pos[1] = 0;
}

void CursorPositionHandler::callback(double xpos, double ypos)
{
  if (!m_disabled)
  {
    // workaround for large x,y offsets after window gets focus. seems to be a glfw bug
    // https://github.com/glfw/glfw/issues/2523
    if (ignore_frames > 0)
    {
      --ignore_frames;
      return;
    }
    m_prev_pos[0] = m_cur_pos[0];
    m_prev_pos[1] = m_cur_pos[1];
    m_cur_pos[0] = xpos;
    m_cur_pos[1] = ypos;
    double diff_x = m_cur_pos[0] - m_prev_pos[0];
    double diff_y = m_cur_pos[1] - m_prev_pos[1];
    if (diff_x != 0.0 || diff_y != 0.0)
    {
      auto& scene = SceneRenderer::instance();
      scene.get_camera().add_to_yaw_and_pitch(diff_x, diff_y);
    }
  }
}
