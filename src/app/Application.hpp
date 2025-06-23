#pragma once

#include "app-context/VulkanApplicationContext.hpp"
#include "utils/event-types/EventType.hpp"
#include "utils/logger/Logger.hpp"
#include "utils/vulkan-wrapper/memory/Image.hpp"
#include "window/KeyboardInfo.hpp"

// 添加脚本引擎相关头文件
#include "dotnet/Engine.hpp"
#include "input/InputSystem.hpp"
#include "dotnet/InputTestSystem.hpp"

#include <memory>

struct ConfigContainer;

class Logger;
class Window;
class FpsSink;
class Renderer;
class ShaderCompiler;
class ImguiManager;

enum class MonkeyControlMode {
    RandomMovement,
    ScriptControl
};

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
    std::unique_ptr<ShaderCompiler> _shaderCompiler       = nullptr;
    std::unique_ptr<Window> _window                       = nullptr;
    std::unique_ptr<Renderer> _renderer                   = nullptr;
    std::unique_ptr<ImguiManager> _imguiManager           = nullptr;
    std::unique_ptr<FpsSink> _fpsSink                     = nullptr;

    // 添加脚本引擎实例
    std::unique_ptr<App> _scriptEngine = nullptr;

    // semaphores and fences for synchronization
    std::vector<VkSemaphore> _imageAvailableSemaphores{};
    std::vector<VkSemaphore> _renderFinishedSemaphores{};
    std::vector<VkFence> _framesInFlightFences{};

    uint32_t _blockStateBits = 0;

    MonkeyControlMode _monkeyControlMode = MonkeyControlMode::RandomMovement;

    void _applicationKeyboardCallback(KeyboardInfo const &keyboardInfo);

    void _createSemaphoresAndFences();
    void _onSwapchainResize();
    void _waitForTheWindowToBeResumed();
    void _drawFrame();
    void _mainLoop();
    void _init();
    void _cleanup();
    void _initScriptEngine();  // 添加脚本引擎初始化方法

    void _onRenderLoopBlockRequest(E_RenderLoopBlockRequest const &event);
    void _buildScene();
};
