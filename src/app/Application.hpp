#pragma once

#include "window/Window.hpp"

#include <memory>

class Application {
public:
  Application();
  void run();

private:
  std::unique_ptr<AppWindow> window;
};
