#pragma once

#include "Core.hpp"

namespace VEngine {
class VENGINE_API Application {
public:
  Application();
  virtual ~Application();

  // disable copy and move constructors
  Application(const Application &)            = delete;
  Application &operator=(const Application &) = delete;
  Application(Application &&)                 = delete;
  Application &operator=(Application &&)      = delete;

  void run();
};

} // namespace VEngine
