#pragma once

#include "app-context/VulkanApplicationContext.hpp"
#include "utils/event-types/EventType.hpp"
#include "utils/logger/Logger.hpp"
#include "utils/vulkan-wrapper/memory/Image.hpp"
#include "utils/vulkan-wrapper/memory/Model.hpp"
#include "window/KeyboardInfo.hpp"

#include <memory>

struct ConfigContainer;

class Logger;
class Window;
class FpsSink;
// class Renderer;
class ShaderCompiler;
class ImguiManager;

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

  std::unique_ptr<VulkanApplicationContext> _appContext = nullptr;
  std::unique_ptr<ConfigContainer> _configContainer     = nullptr;
  // std::unique_ptr<ShaderChangeListener> _shaderFileWatchListener = nullptr;
  std::unique_ptr<ShaderCompiler> _shaderCompiler = nullptr;
  std::unique_ptr<Window> _window                 = nullptr;
  // std::unique_ptr<Renderer> _renderer             = nullptr;
  std::unique_ptr<ImguiManager> _imguiManager = nullptr;
  std::unique_ptr<FpsSink> _fpsSink           = nullptr;
  std::unique_ptr<Model> _model = nullptr;
  struct {
    std::unique_ptr<Image> baseColor;
    std::unique_ptr<Image> normalMap;
    std::unique_ptr<Image> metalRoughness;
  } _images;

  // semaphores and fences for synchronization
  std::vector<VkSemaphore> _imageAvailableSemaphores{};
  std::vector<VkSemaphore> _renderFinishedSemaphores{};
  std::vector<VkFence> _framesInFlightFences{};

  uint32_t _blockStateBits = 0;

  void _applicationKeyboardCallback(KeyboardInfo const &keyboardInfo);

  void _createSemaphoresAndFences();
  void _onSwapchainResize();
  void _waitForTheWindowToBeResumed();
  void _drawFrame();
  void _mainLoop();
  void _init();
  void _cleanup();

  void _onRenderLoopBlockRequest(E_RenderLoopBlockRequest const &event);
  void _buildScene();
};
