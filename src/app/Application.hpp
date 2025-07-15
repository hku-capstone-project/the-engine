#pragma once

#include "app-context/VulkanApplicationContext.hpp"
#include "utils/event-types/EventType.hpp"
#include "utils/logger/Logger.hpp"
#include "utils/vulkan-wrapper/memory/Image.hpp"
#include "utils/vulkan-wrapper/memory/Model.hpp"
#include "window/KeyboardInfo.hpp"

#include <memory>
#include <chrono>
#include <vector>

struct ConfigContainer;

class Logger;
class Window;
class FpsSink;
class Renderer;
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
    std::unique_ptr<ShaderCompiler> _shaderCompiler       = nullptr;
    std::unique_ptr<Window> _window                       = nullptr;
    std::unique_ptr<Renderer> _renderer                   = nullptr;
    std::unique_ptr<ImguiManager> _imguiManager           = nullptr;
    std::unique_ptr<FpsSink> _fpsSink                     = nullptr;

    // semaphores and fences for synchronization
    std::vector<VkSemaphore> _imageAvailableSemaphores{};
    std::vector<VkSemaphore> _renderFinishedSemaphores{};
    std::vector<VkFence> _framesInFlightFences{};

    uint32_t _blockStateBits = 0;

    // Frame timing variables
    struct FrameTimings {
        double pollEvents = 0.0;
        double updateInput = 0.0;
        double runtimeUpdate = 0.0;
        double imguiDraw = 0.0;
        double processInput = 0.0;
        double drawFrame = 0.0;
        double total = 0.0;
        
        // Detailed renderer timing breakdown
        struct RendererTimings {
            double commandBufferSetup = 0.0;
            double entityGrouping = 0.0;
            double instanceDataPrep = 0.0;
            double bufferUpdates = 0.0;
            double gpuCommandRecording = 0.0;
            double commandBufferFinish = 0.0;
            double totalDrawFrame = 0.0;
        } rendererTimings;
        
        // Detailed _drawFrame timing breakdown
        struct DrawFrameBreakdown {
            double fenceWait = 0.0;
            double acquireImage = 0.0;
            double entityDataCollection = 0.0;
            double cameraUpdate = 0.0;
            double rendererDrawFrame = 0.0;
            double imguiCommandBuffer = 0.0;
            double queueSubmit = 0.0;
            double queuePresent = 0.0;
        } drawFrameBreakdown;
    };
    
    std::vector<FrameTimings> _frameTimings;
    size_t _frameCount = 0;
    static constexpr size_t MAX_TIMING_FRAMES = 1000;
    
    // Store last draw frame breakdown for access from main loop
    FrameTimings::DrawFrameBreakdown _lastDrawFrameBreakdown;

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
    
    // Timing measurement helpers
    void _printTimingResults();
    double _getTimeInMilliseconds(std::chrono::steady_clock::time_point start, std::chrono::steady_clock::time_point end);
};
