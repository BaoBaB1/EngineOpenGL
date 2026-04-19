#pragma once

#include "core/Singletone.hpp"
#include "core/Event.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <stack>

struct GLFWwindow;

namespace fury
{
  // clang-format off
  constexpr static std::string_view movement_action_names[] = {
      "MoveForward", "MoveBackward", "MoveLeft", "MoveRight", "MoveUp", "MoveDown"
  };
    
  constexpr static std::string_view input_context_names[] = {
      "FreeCam", "UI"
  };
  // clang-format on

  enum InputCode
  {
    FURY_KEY_INPUTS_BEGIN,
    FURY_KEY_W,
    FURY_KEY_A,
    FURY_KEY_S,
    FURY_KEY_D,
    FURY_KEY_R,
    FURY_KEY_O,
    FURY_KEY_P,
    FURY_KEY_L,
    FURY_KEY_Q,
    FURY_KEY_M,
    FURY_KEY_T,
    FURY_KEY_ARROW_UP,
    FURY_KEY_ARROW_DOWN,
    FURY_KEY_ARROW_LEFT,
    FURY_KEY_ARROW_RIGHT,
    FURY_KEY_LEFT_SHIFT,
    FURY_KEY_SPACE,
    FURY_KEY_LEFT_CTRL,
    FURY_KEY_ESC,
    FURY_KEY_GRAVE_ACCENT,
    FURY_KEY_BACKSPACE,
    FURY_KEY_INPUTS_END,

    FURY_MOUSE_INPUTS_BEGIN,
    FURY_MOUSE_BUTTON_LEFT,
    FURY_MOUSE_BUTTON_RIGHT,
    FURY_MOUSE_INPUTS_END,
  };

  struct Binding
  {
    InputCode input_code;
    float scale = 1;
  };

  struct Action
  {
    std::string name;
    std::vector<Binding> bindings;
  };

  struct InputContext
  {
    std::string name;
    std::unordered_map<std::string, Action> actions;
  };

  class WindowGLFW;

  class InputSystem : public Singletone<InputSystem>
  {
  public:
    void init(fury::WindowGLFW* window);
    const InputContext* get_active_input_context() const { return m_context_stack.top(); }
    void push_context(std::string_view name);
    void pop_context();
    bool is_action_active(const std::string& action) const;
    float get_axis_value(const std::string& action) const;
    Event<InputCode, int, int> on_mouse_button_clicked;
    Event<double, double, double, double> on_cursor_moved;
    Event<InputCode> on_keyboard_button_clicked;
    Event<InputCode> on_keyboard_button_released;
  private:
    void init_keyboard_input(fury::WindowGLFW* window);
    void init_mouse_input(fury::WindowGLFW* window);
    void setup_callbacks(fury::WindowGLFW* window);
    void mouse_click_callback(GLFWwindow* window, int button, int action, int mods);
    void cursor_move_callback(GLFWwindow* window, double xpos, double ypos);
    void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    void create_default_bindings(std::string_view context_name, InputContext& context);
  private:
    std::unordered_map<InputCode, float> m_input_states;
    std::vector<InputContext> m_contexts;
    std::stack<InputContext*> m_context_stack;
    double m_prev_cursor_pos[2] = {};
  };
}; // namespace fury
