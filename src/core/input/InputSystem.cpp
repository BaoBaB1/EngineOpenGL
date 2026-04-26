#include "InputSystem.hpp"
#include "core/WindowGLFW.hpp"
#include "core/Logger.hpp"
#include <GLFW/glfw3.h>

namespace fury
{
  std::map<int, InputCode> mapped_inputs = {
    { GLFW_KEY_W, InputCode::FURY_KEY_W },
    { GLFW_KEY_A, InputCode::FURY_KEY_A },
    { GLFW_KEY_S, InputCode::FURY_KEY_S },
    { GLFW_KEY_D, InputCode::FURY_KEY_D },
    { GLFW_KEY_T, InputCode::FURY_KEY_T },
    { GLFW_KEY_R, InputCode::FURY_KEY_R },
    { GLFW_KEY_O, InputCode::FURY_KEY_O },
    { GLFW_KEY_P, InputCode::FURY_KEY_P },
    { GLFW_KEY_L, InputCode::FURY_KEY_L },
    { GLFW_KEY_Q, InputCode::FURY_KEY_Q },
    { GLFW_KEY_M, InputCode::FURY_KEY_M },
    { GLFW_KEY_UP, InputCode::FURY_KEY_ARROW_UP },
    { GLFW_KEY_DOWN, InputCode::FURY_KEY_ARROW_DOWN },
    { GLFW_KEY_LEFT, InputCode::FURY_KEY_ARROW_LEFT },
    { GLFW_KEY_RIGHT, InputCode::FURY_KEY_ARROW_RIGHT },
    { GLFW_KEY_LEFT_SHIFT, InputCode::FURY_KEY_LEFT_SHIFT },
    { GLFW_KEY_SPACE, InputCode::FURY_KEY_SPACE },
    { GLFW_KEY_LEFT_CONTROL, InputCode::FURY_KEY_LEFT_CTRL },
    { GLFW_KEY_ESCAPE, InputCode::FURY_KEY_ESC },
    { GLFW_KEY_GRAVE_ACCENT, InputCode::FURY_KEY_GRAVE_ACCENT },
    { GLFW_KEY_BACKSPACE, InputCode::FURY_KEY_BACKSPACE },

    { GLFW_MOUSE_BUTTON_1, InputCode::FURY_MOUSE_BUTTON_LEFT },
    { GLFW_MOUSE_BUTTON_2, InputCode::FURY_MOUSE_BUTTON_RIGHT },
  };

  void InputSystem::init(WindowGLFW* window)
  {
    init_keyboard_input(window);
    init_mouse_input(window);
    setup_callbacks(window);
    m_contexts.reserve(std::size(input_context_names));
    for (std::string_view context_name : input_context_names)
    {
      InputContext& input_context = m_contexts.emplace_back();
      input_context.name = context_name;
      create_default_bindings(context_name, input_context);
      if (context_name == "FreeCam")
      {
        // Initially we are in free camera mode
        m_context_stack.push(&input_context);
      }
    }
  }

  void InputSystem::push_context(std::string_view name)
  {
    bool ok = false;
    for (InputContext& ctx : m_contexts)
    {
      if (ctx.name == name)
      {
        ok = true;
        m_context_stack.push(&ctx);
        break;
      }
    }
    if (!ok)
    {
      Logger::warn("Failed to find and push input context with name {}", name);
    }
  }

  void InputSystem::pop_context()
  {
    if (m_context_stack.empty()) [[unlikely]]
    {
      Logger::error("Trying to pop input context with empty context stack!");
      return;
    }
    m_context_stack.pop();
  }

  bool InputSystem::is_action_active(const std::string& action) const
  {
    return get_axis_value(action) != 0.f;
  }

  float InputSystem::get_axis_value(const std::string& action) const
  {
    InputContext* context = m_context_stack.top();
    auto it = context->actions.find(action);
    float value = 0;
    if (auto it = context->actions.find(action); it != context->actions.end())
    {
      for (const auto& binding : it->second.bindings)
      {
        if (auto it2 = m_input_states.find(binding.input_code); it2 != m_input_states.end())
        {
          value += it2->second * binding.scale;
        }
      }
    }
    return value;
  }

  void InputSystem::init_keyboard_input(WindowGLFW* window)
  {
    auto key_callback = [](GLFWwindow* window, int key, int scancode, int action, int mods)
    {
      InputSystem::instance().key_callback(window, key, scancode, action, mods);
    };
    for (int i = static_cast<int>(InputCode::FURY_KEY_INPUTS_BEGIN) + 1;
         i < static_cast<int>(InputCode::FURY_KEY_INPUTS_END); i++)
    {
      m_input_states[static_cast<InputCode>(i)] = 0;
    }
    glfwSetKeyCallback(window->gl_window(), key_callback);
  }

  void InputSystem::init_mouse_input(WindowGLFW* window)
  {
    auto mouse_click_callback = [](GLFWwindow* window, int button, int action, int mods)
    {
      InputSystem::instance().mouse_click_callback(window, button, action, mods);
    };
    auto cursor_move_callback = [](GLFWwindow* window, double xpos, double ypos)
    {
      InputSystem::instance().cursor_move_callback(window, xpos, ypos);
    };

    for (int i = static_cast<int>(InputCode::FURY_MOUSE_INPUTS_BEGIN) + 1;
         i < static_cast<int>(InputCode::FURY_MOUSE_INPUTS_END); i++)
    {
      m_input_states[static_cast<InputCode>(i)] = 0;
    }
    glfwGetCursorPos(window->gl_window(), &m_prev_cursor_pos[0], &m_prev_cursor_pos[1]);
    glfwSetMouseButtonCallback(window->gl_window(), mouse_click_callback);
    glfwSetCursorPosCallback(window->gl_window(), cursor_move_callback);
  }

  void InputSystem::setup_callbacks(fury::WindowGLFW* window)
  {
    window->on_window_cursor_entered += new FunctionListener(std::function(
        [=](bool entered)
        {
          if (entered)
          {
            // m_ignore_frames = 3;
            glfwGetCursorPos(window->gl_window(), &m_prev_cursor_pos[0], &m_prev_cursor_pos[1]);
          }
        }));

    window->on_window_focus_changed += new FunctionListener(std::function(
        [=](bool focused)
        {
          if (focused)
          {
            // m_ignore_frames = 3;
            glfwGetCursorPos(window->gl_window(), &m_prev_cursor_pos[0], &m_prev_cursor_pos[1]);
          }
        }));
  }

  void InputSystem::mouse_click_callback(GLFWwindow* window, int button, int action, int mods)
  {
    if (auto iter = mapped_inputs.find(button); iter != mapped_inputs.end())
    {
      InputSystem& is = InputSystem::instance();
      const InputCode key_code = iter->second;
      if (action == GLFW_PRESS)
      {
        is.m_input_states[key_code] = 1;
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        is.on_mouse_button_clicked.notify(key_code, x, y);
      }
      else if (action == GLFW_RELEASE)
      {
        is.m_input_states[key_code] = 0;
      }
    }
  }

  void InputSystem::cursor_move_callback(GLFWwindow* window, double xpos, double ypos)
  {
    const double dx = std::abs(xpos - m_prev_cursor_pos[0]);
    const double dy = std::abs(ypos - m_prev_cursor_pos[1]);
    // GLFW bug..................
    // https://github.com/glfw/glfw/issues/2523
    if (dx > 100 || dy > 100)
    {
      m_prev_cursor_pos[0] = xpos;
      m_prev_cursor_pos[1] = ypos;
      return;
    }
    // seems like setting cursor mode to GLFW_CURSOR_DISABLED recenters cursor pos
    // and sometimes(?) indirectly triggers cursor callback without actual mouse movement
    if (xpos == m_prev_cursor_pos[0] && ypos == m_prev_cursor_pos[1])
    {
      return;
    }
    double prevx = m_prev_cursor_pos[0], prevy = m_prev_cursor_pos[1];
    m_prev_cursor_pos[0] = xpos;
    m_prev_cursor_pos[1] = ypos;
    on_cursor_moved.notify(xpos, ypos, prevx, prevy);
  }

  void InputSystem::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
  {
    if (auto iter = mapped_inputs.find(key); iter != mapped_inputs.end())
    {
      const InputCode key_code = iter->second;
      switch (action)
      {
      case GLFW_RELEASE:
        m_input_states[key_code] = 0;
        on_keyboard_button_released.notify(key_code);
        break;
      case GLFW_PRESS:
        m_input_states[key_code] = 1;
        on_keyboard_button_clicked.notify(key_code);
        break;
      }
    }
  }

  void InputSystem::create_default_bindings(std::string_view context_name, InputContext& context)
  {
    context.actions.clear();
    if (context_name == "FreeCam")
    {
      for (std::string_view action_name : movement_action_names)
      {
        Action& action = context.actions[std::string(action_name)];
        action.name = action_name;
        if (action.name == "MoveForward")
        {
          action.bindings.push_back(Binding { .input_code = InputCode::FURY_KEY_W });
          action.bindings.push_back(Binding { .input_code = InputCode::FURY_KEY_ARROW_UP });
        }
        else if (action.name == "MoveBackward")
        {
          action.bindings.push_back(Binding { .input_code = InputCode::FURY_KEY_S });
          action.bindings.push_back(Binding { .input_code = InputCode::FURY_KEY_ARROW_DOWN });
        }
        else if (action.name == "MoveLeft")
        {
          action.bindings.push_back(Binding { .input_code = InputCode::FURY_KEY_A });
          action.bindings.push_back(Binding { .input_code = InputCode::FURY_KEY_ARROW_LEFT });
        }
        else if (action.name == "MoveRight")
        {
          action.bindings.push_back(Binding { .input_code = InputCode::FURY_KEY_D });
          action.bindings.push_back(Binding { .input_code = InputCode::FURY_KEY_ARROW_RIGHT });
        }
        else if (action.name == "MoveUp")
        {
          action.bindings.push_back(Binding { .input_code = InputCode::FURY_KEY_SPACE });
        }
        else if (action.name == "MoveDown")
        {
          action.bindings.push_back(Binding { .input_code = InputCode::FURY_KEY_LEFT_CTRL });
        }
        else if (action.name == "SpeedUp")
        {
          action.bindings.push_back(Binding { .input_code = InputCode::FURY_KEY_LEFT_SHIFT, .scale = 5 });
        }
      }
    }
    else if (context_name == "UI")
    {
    }
  }
}; // namespace fury
