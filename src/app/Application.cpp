#include "Application.hpp"
#include "window/Window.hpp"

#include <memory>

Application::Application() { auto window = std::make_unique<AppWindow>("Learn Daxa", 860, 640); }

void Application::run() {
  // Create a window
  auto window = AppWindow("Learn Daxa", 860, 640);

  // Daxa rendering initialization code goes here...

  // Main loop
  while (!window.should_close()) {
    window.update();
  }
}
