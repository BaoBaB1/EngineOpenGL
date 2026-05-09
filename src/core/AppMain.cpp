#include "Application.hpp"
#include "Logger.hpp"

int main()
{
  try
  {
    auto& app = fury::Application::instance();
    app.run();
  }
  catch (const std::exception& e)
  {
    fury::Logger::critical("Caught exception {}", e.what());
    return -1;
  }
  return 0;
}
