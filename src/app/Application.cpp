#include "Application.hpp"
#include "BlockState.hpp"
#include "config-container/ConfigContainer.hpp"
#include "config-container/sub-config/ApplicationInfo.hpp"
#include "imgui-manager/gui-manager/ImguiManager.hpp"
#include "renderer/Renderer.hpp"
#include "utils/event-dispatcher/GlobalEventDispatcher.hpp"
#include "utils/event-types/EventType.hpp"
#include "utils/fps-sink/FpsSink.hpp"
#include "utils/logger/Logger.hpp"
#include "utils/shader-compiler/ShaderCompiler.hpp"
#include "window/Window.hpp"

#include "dotnet/TestManaged.hpp"

#include <memory>

Application::Application(Logger *logger) : _logger(logger) {
    _appContext      = std::make_unique<VulkanApplicationContext>();
    _configContainer = std::make_unique<ConfigContainer>(_logger);

    _shaderCompiler = std::make_unique<ShaderCompiler>(
        logger, [this](std::string const &fullPathToIncludedShaderFile) {});

    _window = std::make_unique<Window>(WindowStyle::kMaximized, logger);

    VulkanApplicationContext::GraphicsSettings settings{};
    settings.isFramerateLimited = _configContainer->applicationInfo->isFramerateLimited;
    _appContext->init(_logger, _window->getGlWindow(), &settings);

    _imguiManager = std::make_unique<ImguiManager>(_appContext.get(), _window.get(), _logger,
                                                   _configContainer.get());

    _fpsSink = std::make_unique<FpsSink>();

    _renderer = std::make_unique<Renderer>(
        _appContext.get(), _logger, _configContainer->applicationInfo->framesInFlight,
        _shaderCompiler.get(), _window.get(), _configContainer.get());

    _initScriptEngine();

    _init();

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

    // 初始化InputSystem
    InputSystem::Initialize();

    // attach application-level keyboard listeners
    _window->addKeyboardCallback(
        [this](KeyboardInfo const &keyboardInfo) { _applicationKeyboardCallback(keyboardInfo); });
    
    // 运行脚本引擎的startup systems
    if (_scriptEngine) {
        for (auto &s : _scriptEngine->startSystems) {
            s();
        }
        _logger->info("Script engine startup systems executed");
        
        // 直接创建猴子entity用于渲染
        auto monkeyEntity = _scriptEngine->registry.create();
        Transform monkeyTransform;
        monkeyTransform.position = glm::vec3(0.0f, 1.0f, 0.0f);
        _scriptEngine->registry.emplace<Transform>(monkeyEntity, monkeyTransform);
        
        Mesh monkeyMesh;
        monkeyMesh.modelId = 0;  // 猴子模型ID
        _scriptEngine->registry.emplace<Mesh>(monkeyEntity, monkeyMesh);
        
        _logger->info("Monkey entity created directly in Application");
        
        // 输出控制说明
        _logger->info("=== CONTROLS ===");
        _logger->info("Camera Control:");
        _logger->info("  Arrow Keys - Move forward/back/left/right");
        _logger->info("  Space/Ctrl - Move up/down");
        _logger->info("  Mouse      - Look around (press E to toggle mouse capture)");
        _logger->info("");
        _logger->info("Monkey Control Mode:");
        _logger->info("  F1  - Random movement mode (default)");
        _logger->info("  F2  - Script control mode (WASD to control monkey)");
        _logger->info("  In script mode: WASD/QZ - Move monkey, Space - Print position");
        _logger->info("");
        _logger->info("Other:");
        _logger->info("  E   - Toggle mouse cursor");
        _logger->info("  ESC - Exit");
        _logger->info("================");
    }
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
    
    if (keyboardInfo.isKeyPressed(GLFW_KEY_F1)) {
        if (_monkeyControlMode != MonkeyControlMode::RandomMovement) {
            _monkeyControlMode = MonkeyControlMode::RandomMovement;
            _logger->info("Switched to Random Movement mode");
            
            // 清除脚本系统的update systems
            if (_scriptEngine) {
                _scriptEngine->updateSystems.clear();
            }
        }
        return;
    }
    
    if (keyboardInfo.isKeyPressed(GLFW_KEY_F2)) {
        if (_monkeyControlMode != MonkeyControlMode::ScriptControl) {
            _monkeyControlMode = MonkeyControlMode::ScriptControl;
            _logger->info("Switched to Script Control mode (use WASD to control monkey)");
            
            // 设置脚本控制系统
            if (_scriptEngine) {
                _scriptEngine->updateSystems.clear();
                InputTestSystem::CreatePlayerControllerSystem(*_scriptEngine);
            }
        }
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

    // 更新InputSystem状态
    InputSystem::UpdateFrame(_window->getKeyboardInfo(), _window->getCursorInfo());
    
    // 计算deltaTime并运行脚本引擎的update systems
    static auto lastFrameTime = std::chrono::steady_clock::now();
    auto currentTime = std::chrono::steady_clock::now();
    float deltaTime = std::chrono::duration<float>(currentTime - lastFrameTime).count();
    lastFrameTime = currentTime;

    glm::vec3 currentPosition = glm::vec3(0.0f, 1.0f, 0.0f);  // 默认位置
    
    // 更新脚本系统
    if (_scriptEngine) {
        // 运行脚本引擎的update systems
        for (auto &s : _scriptEngine->updateSystems) {
            s(deltaTime);
        }
        
        // 根据控制模式更新猴子位置
        if (_monkeyControlMode == MonkeyControlMode::RandomMovement) {
            // 随机更新猴子位置（小范围移动）
            auto view = _scriptEngine->registry.view<Transform, Mesh>();
            for (auto entity : view) {
                auto& transform = view.get<Transform>(entity);
                auto& mesh = view.get<Mesh>(entity);
                
                if (mesh.modelId == 0) {  // 猴子模型
                    static float time = 0.0f;
                    time += 0.02f;  // 缓慢时间增量
                    
                    // 使用正弦波的组合实现随机移动
                    transform.position.x = 1.5f * std::sin(time * 0.7f) * std::cos(time * 0.5f);
                    transform.position.y = 0.5f * std::sin(time * 1.3f) + 1.0f;  // y轴基础高度1.0
                    transform.position.z = 1.2f * std::cos(time * 0.9f) * std::sin(time * 0.4f);
                    
                    currentPosition = transform.position;
                    break;
                }
            }
        } else {
            // 脚本控制模式下，位置由脚本系统控制
            // 只需要获取当前位置传递给渲染器
            auto view = _scriptEngine->registry.view<Transform, Mesh>();
            for (auto entity : view) {
                auto& transform = view.get<Transform>(entity);
                auto& mesh = view.get<Mesh>(entity);
                
                if (mesh.modelId == 0) {  // 猴子模型
                    currentPosition = transform.position;
                    break;
                }
            }
        }
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

void Application::_initScriptEngine() {
    _scriptEngine = std::make_unique<App>();
    
    if (init_script_engine(*_scriptEngine)) {
        _logger->info("Script engine initialized successfully");
    } else {
        _logger->error("Failed to initialize script engine");
        _scriptEngine.reset();  // 清理失败的引擎
    }
}
