#include "CursorPositionHandler.hpp"
#include "core/SceneRenderer.hpp"
#include "core/WindowGLFW.hpp"
#include <GLFW/glfw3.h>

namespace fury
{
  CursorPositionHandler::CursorPositionHandler(WindowGLFW* window) : UserInputHandler(window, HandlerType::CURSOR_POSITION)
  {
    auto callback = [](GLFWwindow* window, double xpos, double ypos)
      {
        WindowGLFW* window_glfw = static_cast<WindowGLFW*>(glfwGetWindowUserPointer(window));
        window_glfw->get_input_handler<CursorPositionHandler>(HandlerType::CURSOR_POSITION)->callback(xpos, ypos);
      };
    glfwSetCursorPosCallback(m_window->gl_window(), callback);
    glfwGetCursorPos(m_window->gl_window(), &m_cur_pos[0], &m_cur_pos[1]);
    m_prev_pos[0] = m_prev_pos[1] = 0;
  }

  void CursorPositionHandler::callback(double xpos, double ypos)
  {
    // workaround for large x,y offsets after window gets focus. seems to be a glfw bug
    // https://github.com/glfw/glfw/issues/2523
    if (m_ignore_frames > 0)
    {
      --m_ignore_frames;
      return;
    }
    // seems like setting cursor mode to GLFW_CURSOR_DISABLED recenters cursor pos to the center of the window, 
    // and sometimes(?) indirectly triggers cursor callback without actual mouse movement
    if (xpos == m_cur_pos[0] && ypos == m_cur_pos[1])
      return;
    m_prev_pos[0] = m_cur_pos[0];
    m_prev_pos[1] = m_cur_pos[1];
    m_cur_pos[0] = xpos;
    m_cur_pos[1] = ypos;
    on_cursor_position_change.notify(m_cur_pos[0], m_cur_pos[1], m_prev_pos[0], m_prev_pos[1]);
  }
};
