#include "Application.hpp"
#include "utils/logger/Logger.hpp"
#include "window/Window.hpp"

#include <memory>

Application::Application(Logger *logger) : _logger(logger) {
  _appContext = std::make_unique<VulkanApplicationContext>();
  _window     = std::make_unique<Window>(WindowStyle::kMaximized, logger);

  VulkanApplicationContext::GraphicsSettings settings{};
  settings.isFramerateLimited = true;
  _appContext->init(_logger, _window->getGlWindow(), &settings);

  _logger->info("Application is created successfully!");
}

Application::~Application() { _logger->info("Application is destroyed"); }

void Application::run() {}
