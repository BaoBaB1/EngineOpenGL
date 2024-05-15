#include "MainWindow.hpp"
#include "KeyboardHandler.hpp"
#include "CursorPositionHandler.hpp"
#include "MouseInputHandler.hpp"
#include "Debug.hpp"

extern int ignore_frames = 3;

static void window_focus_callback(GLFWwindow* window, int focused)
{
  if (focused)
  {
    ignore_frames = 3;
  }
}

MainWindow::MainWindow(int width, int height, const char* title) :
  m_width(width), m_height(height), m_title(title)
{
  // Tell GLFW what version of OpenGL we are using 
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
  // Tell GLFW we are using the CORE profile (only modern functions)
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
  if (m_window == nullptr) {
    DEBUG("Failed to create GLFW window" << std::endl);
    glfwTerminate();
  }
  glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetCursorPos(m_window, m_width / 2., m_height / 2.);
  m_input_handlers[UserInputHandler::HandlerType::KEYBOARD] = std::make_unique<KeyboardHandler>(this);
  m_input_handlers[UserInputHandler::HandlerType::CURSOR_POSITION] = std::make_unique<CursorPositionHandler>(this);
  m_input_handlers[UserInputHandler::HandlerType::MOUSE_INPUT] = std::make_unique<MouseInputHandler>(this);
  glfwMakeContextCurrent(m_window);
  // disable vsync
  glfwSwapInterval(0);
  glfwSetWindowSizeLimits(m_window, 1600, 900, GLFW_DONT_CARE, GLFW_DONT_CARE);
  glfwSetWindowFocusCallback(m_window, window_focus_callback);

  glfwSetWindowUserPointer(m_window, this);
  auto window_size_change_callback = [](GLFWwindow* window, int width, int height)
    {
      static_cast<MainWindow*>(glfwGetWindowUserPointer(window))->on_window_size_change.notify(width, height);
    };
  glfwSetWindowSizeCallback(m_window, window_size_change_callback);

  gladLoadGL();
  glViewport(0, 0, m_width, m_height);
}

void MainWindow::notify(IObserver* observer, bool enable)
{
  observer->notify(enable);
}

void MainWindow::notify_all(bool enable)
{
  for (const auto& phandler : m_input_handlers)
    phandler.second->notify(enable);
}

MainWindow::~MainWindow()
{
  glfwDestroyWindow(m_window);
  glfwTerminate();
}
