#pragma once

#include "app-context/VulkanApplicationContext.hpp"

#include <memory>

class Logger;
class Window;
class Application {
public:
  Application(Logger *logger);
  ~Application();

  // disable move and copy
  Application(const Application &)            = delete;
  Application &operator=(const Application &) = delete;
  Application(Application &&)                 = delete;
  Application &operator=(Application &&)      = delete;

  void run();

private:
  Logger *_logger;

  std::unique_ptr<Window> _window = nullptr;

  std::unique_ptr<VulkanApplicationContext> _appContext;
};
