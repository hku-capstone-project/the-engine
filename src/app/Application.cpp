#include "Application.hpp"
#include "BlockState.hpp"
#include "config-container/ConfigContainer.hpp"
#include "config-container/sub-config/ApplicationInfo.hpp"
#include "dotnet/Components.hpp"
#include "dotnet/RuntimeApplication.hpp"
#include "dotnet/RuntimeBridge.hpp"
#include "imgui-manager/gui-manager/ImguiManager.hpp"
#include "renderer/Renderer.hpp"
#include "utils/event-dispatcher/GlobalEventDispatcher.hpp"
#include "utils/event-types/EventType.hpp"
#include "utils/fps-sink/FpsSink.hpp"
#include "utils/logger/Logger.hpp"
#include "utils/shader-compiler/ShaderCompiler.hpp"
#include "window/Window.hpp"

#include <memory>

Application::Application(Logger *logger) : _logger(logger) {
    // bootstrap the runtime application, to be ready to connect with the managed code
    RuntimeBridge::bootstrap(logger);

    _appContext      = std::make_unique<VulkanApplicationContext>();
    _configContainer = std::make_unique<ConfigContainer>(_logger);

    _shaderCompiler = std::make_unique<ShaderCompiler>(
        logger, [this](std::string const &fullPathToIncludedShaderFile) {});

    _window = std::make_unique<Window>(WindowStyle::kMaximized, logger);

    // Set window reference for runtime application to access keyboard input
    RuntimeBridge::getRuntimeApplication().setWindow(_window.get());

    VulkanApplicationContext::GraphicsSettings settings{};
    settings.isFramerateLimited = _configContainer->applicationInfo->isFramerateLimited;
    _appContext->init(_logger, _window->getGlWindow(), &settings);

    _imguiManager = std::make_unique<ImguiManager>(_appContext.get(), _window.get(), _logger,
                                                   _configContainer.get());

    _fpsSink = std::make_unique<FpsSink>();

    _renderer = std::make_unique<Renderer>(
        _appContext.get(), _logger, _configContainer->applicationInfo->framesInFlight,
        _shaderCompiler.get(), _window.get(), _configContainer.get());

    _init();

    // call every startup system after everything is initialized
    RuntimeBridge::getRuntimeApplication().start();

    GlobalEventDispatcher::get()
        .sink<E_RenderLoopBlockRequest>()
        .connect<&Application::_onRenderLoopBlockRequest>(this);
}

void Application::_onRenderLoopBlockRequest(E_RenderLoopBlockRequest const &event) {
    _blockStateBits |= event.blockStateBits;
}

Application::~Application() { _cleanup(); }

void Application::run() { _mainLoop(); }

void Application::_init() {
    _imguiManager->init();
    _createSemaphoresAndFences();

    // attach application-level keyboard listeners
    _window->addKeyboardCallback(
        [this](KeyboardInfo const &keyboardInfo) { _applicationKeyboardCallback(keyboardInfo); });
}

void Application::_applicationKeyboardCallback(KeyboardInfo const &keyboardInfo) {
    if (keyboardInfo.isKeyPressed(GLFW_KEY_ESCAPE)) {
        glfwSetWindowShouldClose(_window->getGlWindow(), 1);
        return;
    }

    if (keyboardInfo.isKeyPressed(GLFW_KEY_E)) {
        _window->toggleCursor();
        return;
    }
}

void Application::_createSemaphoresAndFences() {
    _imageAvailableSemaphores.resize(_configContainer->applicationInfo->framesInFlight);
    _renderFinishedSemaphores.resize(_configContainer->applicationInfo->framesInFlight);
    _framesInFlightFences.resize(_configContainer->applicationInfo->framesInFlight);

    VkSemaphoreCreateInfo semaphoreInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

    VkFenceCreateInfo fenceCreateInfoPreSignalled{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fenceCreateInfoPreSignalled.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < _configContainer->applicationInfo->framesInFlight; i++) {
        vkCreateSemaphore(_appContext->getDevice(), &semaphoreInfo, nullptr,
                          &_imageAvailableSemaphores[i]);
        vkCreateSemaphore(_appContext->getDevice(), &semaphoreInfo, nullptr,
                          &_renderFinishedSemaphores[i]);
        vkCreateFence(_appContext->getDevice(), &fenceCreateInfoPreSignalled, nullptr,
                      &_framesInFlightFences[i]);
    }
}

void Application::_waitForTheWindowToBeResumed() {
    int windowWidth  = 0;
    int windowHeight = 0;
    _window->getWindowDimension(windowWidth, windowHeight);

    while (windowWidth == 0 || windowHeight == 0) {
        glfwWaitEvents();

        _window->getWindowDimension(windowWidth, windowHeight);
    }
}

void Application::_onSwapchainResize() {
    _logger->info("Swapchain resized");
    _appContext->onSwapchainResize(_configContainer->applicationInfo->isFramerateLimited);
    _imguiManager->onSwapchainResize();
    _renderer->onSwapchainResize();
}

void Application::_mainLoop() {
    static std::chrono::time_point fpsRecordLastTime = std::chrono::steady_clock::now();

    while (glfwWindowShouldClose(_window->getGlWindow()) == 0) {
        glfwPollEvents();

        if (_blockStateBits != 0) {
            vkDeviceWaitIdle(_appContext->getDevice());

            if (_blockStateBits & BlockState::kShaderChanged) {
                GlobalEventDispatcher::get().trigger<E_RenderLoopBlocked>();
                // then some rebuilding will happen
            }

            if (_blockStateBits & BlockState::kWindowResized) {
                _waitForTheWindowToBeResumed();
                _onSwapchainResize();
            }

            // reset the block state and timer
            _blockStateBits   = 0;
            fpsRecordLastTime = std::chrono::steady_clock::now();
            continue;
        }

        auto currentTime  = std::chrono::steady_clock::now();
        auto deltaTime    = currentTime - fpsRecordLastTime;
        fpsRecordLastTime = currentTime;

        float dt = std::chrono::duration<float>(deltaTime).count();
        
        // 更新新的游戏输入系统状态
        _window->updateInputStates();
        
        RuntimeBridge::getRuntimeApplication().update(dt);

        double deltaTimeInSec =
            std::chrono::duration<double, std::chrono::seconds::period>(deltaTime).count();
        fpsRecordLastTime = currentTime;

        _fpsSink->addRecord(1.0F / deltaTimeInSec);
        _imguiManager->draw(_fpsSink.get());
        _renderer->processInput(deltaTimeInSec);

        _drawFrame();
    }

    vkDeviceWaitIdle(_appContext->getDevice());
}

void Application::_cleanup() {
    for (size_t i = 0; i < _configContainer->applicationInfo->framesInFlight; i++) {
        vkDestroySemaphore(_appContext->getDevice(), _renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(_appContext->getDevice(), _imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(_appContext->getDevice(), _framesInFlightFences[i], nullptr);
    }
}

void Application::_drawFrame() {
    static size_t currentFrame = 0;
    vkWaitForFences(_appContext->getDevice(), 1, &_framesInFlightFences[currentFrame], VK_TRUE,
                    UINT64_MAX);
    vkResetFences(_appContext->getDevice(), 1, &_framesInFlightFences[currentFrame]);

    uint32_t imageIndex = 0;
    // this process is fairly quick, but it is related to communicating with the GPU
    // https://stackoverflow.com/questions/60419749/why-does-vkacquirenextimagekhr-never-block-my-thread
    VkResult result =
        vkAcquireNextImageKHR(_appContext->getDevice(), _appContext->getSwapchain(), UINT64_MAX,
                              _imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        return;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        // sub-optimal: a swapchain no longer matches the surface properties
        // exactly, but can still be used to present to the surface successfully
        _logger->error("resizing is not allowed!");
    }

    auto const &reg        = RuntimeBridge::getRuntimeApplication().registry;
    auto const &transforms = reg.view<Transform>();
    // if not null, pick the first one for the currentPosition
    glm::vec3 currentPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    if (transforms.size() > 0) {
        currentPosition = transforms.get<Transform>(transforms.front()).position;
    }

    auto const modelMatrix = glm::translate(glm::mat4(1.0f), currentPosition);
    _renderer->drawFrame(currentFrame, imageIndex, modelMatrix);

    _imguiManager->recordCommandBuffer(currentFrame, imageIndex);
    std::vector<VkCommandBuffer> submitCommandBuffers = {
        _renderer->getDrawingCommandBuffer(currentFrame),
        _renderer->getDeliveryCommandBuffer(imageIndex),
        _imguiManager->getCommandBuffer(currentFrame),
    };

    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    // wait until the image is ready
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores    = &_imageAvailableSemaphores[currentFrame];
    // signal a semaphore after render finished
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = &_renderFinishedSemaphores[currentFrame];
    // wait for no stage
    VkPipelineStageFlags waitStages{VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
    submitInfo.pWaitDstStageMask = &waitStages;

    submitInfo.commandBufferCount = static_cast<uint32_t>(submitCommandBuffers.size());
    submitInfo.pCommandBuffers    = submitCommandBuffers.data();

    vkQueueSubmit(_appContext->getGraphicsQueue(), 1, &submitInfo,
                  _framesInFlightFences[currentFrame]);

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &_renderFinishedSemaphores[currentFrame];
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &_appContext->getSwapchain();
    presentInfo.pImageIndices      = &imageIndex;
    presentInfo.pResults           = nullptr;

    vkQueuePresentKHR(_appContext->getPresentQueue(), &presentInfo);

    currentFrame = (currentFrame + 1) % _configContainer->applicationInfo->framesInFlight;
}
