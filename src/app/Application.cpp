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

    _init();

    // call every startup system to register meshes BEFORE creating the renderer
    RuntimeBridge::getRuntimeApplication().start();

    _renderer = std::make_unique<Renderer>(
        _appContext.get(), _logger, _configContainer->applicationInfo->framesInFlight,
        _shaderCompiler.get(), _window.get(), _configContainer.get());

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
    
    // Initialize timing data if frame timing is enabled
    if (_configContainer->applicationInfo->enableFrameTiming) {
        _frameTimings.reserve(MAX_TIMING_FRAMES);
        _frameCount = 0;
    }

    while (glfwWindowShouldClose(_window->getGlWindow()) == 0) {
        auto frameStartTime = std::chrono::steady_clock::now();
        FrameTimings currentFrameTimings{};
        
        // Poll events timing
        auto pollEventsStart = std::chrono::steady_clock::now();
        glfwPollEvents();
        auto pollEventsEnd = std::chrono::steady_clock::now();
        if (_configContainer->applicationInfo->enableFrameTiming) {
            currentFrameTimings.pollEvents = _getTimeInMilliseconds(pollEventsStart, pollEventsEnd);
        }

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
        
        // Update input states timing
        auto updateInputStart = std::chrono::steady_clock::now();
        _window->updateInputStates();
        auto updateInputEnd = std::chrono::steady_clock::now();
        if (_configContainer->applicationInfo->enableFrameTiming) {
            currentFrameTimings.updateInput = _getTimeInMilliseconds(updateInputStart, updateInputEnd);
        }
        
        // Runtime update timing
        auto runtimeUpdateStart = std::chrono::steady_clock::now();
        RuntimeBridge::getRuntimeApplication().update(dt);
        auto runtimeUpdateEnd = std::chrono::steady_clock::now();
        if (_configContainer->applicationInfo->enableFrameTiming) {
            currentFrameTimings.runtimeUpdate = _getTimeInMilliseconds(runtimeUpdateStart, runtimeUpdateEnd);
        }

        double deltaTimeInSec =
            std::chrono::duration<double, std::chrono::seconds::period>(deltaTime).count();
        fpsRecordLastTime = currentTime;

        _fpsSink->addRecord(1.0F / deltaTimeInSec);
        
        // ImGui draw timing
        auto imguiDrawStart = std::chrono::steady_clock::now();
        _imguiManager->draw(_fpsSink.get());
        auto imguiDrawEnd = std::chrono::steady_clock::now();
        if (_configContainer->applicationInfo->enableFrameTiming) {
            currentFrameTimings.imguiDraw = _getTimeInMilliseconds(imguiDrawStart, imguiDrawEnd);
        }
        
        // Process input timing
        auto processInputStart = std::chrono::steady_clock::now();
        _renderer->processInput(deltaTimeInSec);
        auto processInputEnd = std::chrono::steady_clock::now();
        if (_configContainer->applicationInfo->enableFrameTiming) {
            currentFrameTimings.processInput = _getTimeInMilliseconds(processInputStart, processInputEnd);
        }

        // Draw frame timing
        auto drawFrameStart = std::chrono::steady_clock::now();
        _drawFrame();
        auto drawFrameEnd = std::chrono::steady_clock::now();
        if (_configContainer->applicationInfo->enableFrameTiming) {
            currentFrameTimings.drawFrame = _getTimeInMilliseconds(drawFrameStart, drawFrameEnd);
            currentFrameTimings.total = _getTimeInMilliseconds(frameStartTime, drawFrameEnd);
            
            // Collect detailed renderer timing
            const auto& rendererTimings = _renderer->getLastFrameTimings();
            currentFrameTimings.rendererTimings.commandBufferSetup = rendererTimings.commandBufferSetup;
            currentFrameTimings.rendererTimings.entityGrouping = rendererTimings.entityGrouping;
            currentFrameTimings.rendererTimings.instanceDataPrep = rendererTimings.instanceDataPrep;
            currentFrameTimings.rendererTimings.bufferUpdates = rendererTimings.bufferUpdates;
            currentFrameTimings.rendererTimings.gpuCommandRecording = rendererTimings.gpuCommandRecording;
            currentFrameTimings.rendererTimings.commandBufferFinish = rendererTimings.commandBufferFinish;
            currentFrameTimings.rendererTimings.totalDrawFrame = rendererTimings.totalDrawFrame;
            
            // Collect detailed draw frame breakdown
            currentFrameTimings.drawFrameBreakdown = _lastDrawFrameBreakdown;
            
            // Store timing data
            _frameTimings.push_back(currentFrameTimings);
            _frameCount++;
            
            // Check if we've reached the target frame count
            if (_frameCount >= MAX_TIMING_FRAMES) {
                _printTimingResults();
                glfwSetWindowShouldClose(_window->getGlWindow(), 1);
            }
        }
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
    
    // Fence wait timing
    auto fenceWaitStart = std::chrono::steady_clock::now();
    vkWaitForFences(_appContext->getDevice(), 1, &_framesInFlightFences[currentFrame], VK_TRUE,
                    UINT64_MAX);
    vkResetFences(_appContext->getDevice(), 1, &_framesInFlightFences[currentFrame]);
    auto fenceWaitEnd = std::chrono::steady_clock::now();
    
    double fenceWaitTime = 0.0;
    if (_configContainer->applicationInfo->enableFrameTiming) {
        fenceWaitTime = _getTimeInMilliseconds(fenceWaitStart, fenceWaitEnd);
    }

    // Acquire image timing
    auto acquireImageStart = std::chrono::steady_clock::now();
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
    auto acquireImageEnd = std::chrono::steady_clock::now();
    
    double acquireImageTime = 0.0;
    if (_configContainer->applicationInfo->enableFrameTiming) {
        acquireImageTime = _getTimeInMilliseconds(acquireImageStart, acquireImageEnd);
    }

    // Entity data collection timing
    auto entityDataStart = std::chrono::steady_clock::now();
    auto const &reg        = RuntimeBridge::getRuntimeApplication().registry;
    
    // 改为基于ECS实体的渲染系统
    // 遍历所有有Transform和Mesh组件的实体
    auto renderableEntities = reg.view<Transform, Mesh, Material>();
    
    std::vector<std::unique_ptr<Components>> entityRenderData;
    
    for (auto entity : renderableEntities) {
        auto& transform = renderableEntities.get<Transform>(entity);
        auto& mesh = renderableEntities.get<Mesh>(entity);
        auto& material = renderableEntities.get<Material>(entity);

        // 为每个实体创建模型矩阵
        glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), transform.position);
        
        // 存储实体的渲染数据（变换矩阵 + 模型ID）
        Components* comp = new Components();
        comp->transform = transform;
        comp->mesh = mesh;
        comp->material = material;
        entityRenderData.emplace_back(comp);
    }
    auto entityDataEnd = std::chrono::steady_clock::now();
    
    double entityDataTime = 0.0;
    if (_configContainer->applicationInfo->enableFrameTiming) {
        entityDataTime = _getTimeInMilliseconds(entityDataStart, entityDataEnd);
    }

    // Camera update timing
    auto cameraUpdateStart = std::chrono::steady_clock::now();
    auto camEntities = reg.view<Transform, iCamera>();
    
    for (auto entity : camEntities) {
        auto &transform = camEntities.get<Transform>(entity);
        auto &camera = camEntities.get<iCamera>(entity);

        // 更新摄像机位置和投影矩阵
        _renderer->updateCamera(transform, camera);
    }
    auto cameraUpdateEnd = std::chrono::steady_clock::now();
    
    double cameraUpdateTime = 0.0;
    if (_configContainer->applicationInfo->enableFrameTiming) {
        cameraUpdateTime = _getTimeInMilliseconds(cameraUpdateStart, cameraUpdateEnd);
    }
    
    // Renderer draw frame timing
    auto rendererDrawStart = std::chrono::steady_clock::now();
    // 传递实体渲染数据给渲染器
    _renderer->drawFrame(currentFrame, imageIndex, entityRenderData);
    auto rendererDrawEnd = std::chrono::steady_clock::now();
    
    double rendererDrawTime = 0.0;
    if (_configContainer->applicationInfo->enableFrameTiming) {
        rendererDrawTime = _getTimeInMilliseconds(rendererDrawStart, rendererDrawEnd);
    }

    // ImGui command buffer timing
    auto imguiCmdStart = std::chrono::steady_clock::now();
    _imguiManager->recordCommandBuffer(currentFrame, imageIndex);
    auto imguiCmdEnd = std::chrono::steady_clock::now();
    
    double imguiCmdTime = 0.0;
    if (_configContainer->applicationInfo->enableFrameTiming) {
        imguiCmdTime = _getTimeInMilliseconds(imguiCmdStart, imguiCmdEnd);
    }
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

    // Queue submit timing
    auto queueSubmitStart = std::chrono::steady_clock::now();
    vkQueueSubmit(_appContext->getGraphicsQueue(), 1, &submitInfo,
                  _framesInFlightFences[currentFrame]);
    auto queueSubmitEnd = std::chrono::steady_clock::now();
    
    double queueSubmitTime = 0.0;
    if (_configContainer->applicationInfo->enableFrameTiming) {
        queueSubmitTime = _getTimeInMilliseconds(queueSubmitStart, queueSubmitEnd);
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &_renderFinishedSemaphores[currentFrame];
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &_appContext->getSwapchain();
    presentInfo.pImageIndices      = &imageIndex;
    presentInfo.pResults           = nullptr;

    // Queue present timing
    auto queuePresentStart = std::chrono::steady_clock::now();
    vkQueuePresentKHR(_appContext->getPresentQueue(), &presentInfo);
    auto queuePresentEnd = std::chrono::steady_clock::now();
    
    double queuePresentTime = 0.0;
    if (_configContainer->applicationInfo->enableFrameTiming) {
        queuePresentTime = _getTimeInMilliseconds(queuePresentStart, queuePresentEnd);
    }

    // Store timing data for this frame (use static to persist across calls)
    static FrameTimings::DrawFrameBreakdown lastDrawFrameBreakdown;
    if (_configContainer->applicationInfo->enableFrameTiming) {
        lastDrawFrameBreakdown.fenceWait = fenceWaitTime;
        lastDrawFrameBreakdown.acquireImage = acquireImageTime;
        lastDrawFrameBreakdown.entityDataCollection = entityDataTime;
        lastDrawFrameBreakdown.cameraUpdate = cameraUpdateTime;
        lastDrawFrameBreakdown.rendererDrawFrame = rendererDrawTime;
        lastDrawFrameBreakdown.imguiCommandBuffer = imguiCmdTime;
        lastDrawFrameBreakdown.queueSubmit = queueSubmitTime;
        lastDrawFrameBreakdown.queuePresent = queuePresentTime;
    }
    
    // Store in a member variable for access from main loop
    _lastDrawFrameBreakdown = lastDrawFrameBreakdown;

    currentFrame = (currentFrame + 1) % _configContainer->applicationInfo->framesInFlight;
}

double Application::_getTimeInMilliseconds(std::chrono::steady_clock::time_point start, std::chrono::steady_clock::time_point end) {
    return std::chrono::duration<double, std::milli>(end - start).count();
}

void Application::_printTimingResults() {
    if (_frameTimings.empty()) {
        _logger->info("No timing data collected");
        return;
    }
    
    // Calculate averages
    double avgPollEvents = 0.0, avgUpdateInput = 0.0, avgRuntimeUpdate = 0.0;
    double avgImguiDraw = 0.0, avgProcessInput = 0.0, avgDrawFrame = 0.0, avgTotal = 0.0;
    
    // Detailed renderer timing averages
    double avgCmdBufferSetup = 0.0, avgEntityGrouping = 0.0, avgInstanceDataPrep = 0.0;
    double avgBufferUpdates = 0.0, avgGpuCommandRecording = 0.0, avgCmdBufferFinish = 0.0;
    double avgRendererTotal = 0.0;
    
    // Draw frame breakdown averages
    double avgFenceWait = 0.0, avgAcquireImage = 0.0, avgEntityData = 0.0, avgCameraUpdate = 0.0;
    double avgRendererDraw = 0.0, avgImguiCmd = 0.0, avgQueueSubmit = 0.0, avgQueuePresent = 0.0;
    
    for (const auto& timing : _frameTimings) {
        avgPollEvents += timing.pollEvents;
        avgUpdateInput += timing.updateInput;
        avgRuntimeUpdate += timing.runtimeUpdate;
        avgImguiDraw += timing.imguiDraw;
        avgProcessInput += timing.processInput;
        avgDrawFrame += timing.drawFrame;
        avgTotal += timing.total;
        
        // Renderer timing averages
        avgCmdBufferSetup += timing.rendererTimings.commandBufferSetup;
        avgEntityGrouping += timing.rendererTimings.entityGrouping;
        avgInstanceDataPrep += timing.rendererTimings.instanceDataPrep;
        avgBufferUpdates += timing.rendererTimings.bufferUpdates;
        avgGpuCommandRecording += timing.rendererTimings.gpuCommandRecording;
        avgCmdBufferFinish += timing.rendererTimings.commandBufferFinish;
        avgRendererTotal += timing.rendererTimings.totalDrawFrame;
        
        // Draw frame breakdown averages
        avgFenceWait += timing.drawFrameBreakdown.fenceWait;
        avgAcquireImage += timing.drawFrameBreakdown.acquireImage;
        avgEntityData += timing.drawFrameBreakdown.entityDataCollection;
        avgCameraUpdate += timing.drawFrameBreakdown.cameraUpdate;
        avgRendererDraw += timing.drawFrameBreakdown.rendererDrawFrame;
        avgImguiCmd += timing.drawFrameBreakdown.imguiCommandBuffer;
        avgQueueSubmit += timing.drawFrameBreakdown.queueSubmit;
        avgQueuePresent += timing.drawFrameBreakdown.queuePresent;
    }
    
    size_t frameCount = _frameTimings.size();
    avgPollEvents /= frameCount;
    avgUpdateInput /= frameCount;
    avgRuntimeUpdate /= frameCount;
    avgImguiDraw /= frameCount;
    avgProcessInput /= frameCount;
    avgDrawFrame /= frameCount;
    avgTotal /= frameCount;
    
    // Renderer timing averages
    avgCmdBufferSetup /= frameCount;
    avgEntityGrouping /= frameCount;
    avgInstanceDataPrep /= frameCount;
    avgBufferUpdates /= frameCount;
    avgGpuCommandRecording /= frameCount;
    avgCmdBufferFinish /= frameCount;
    avgRendererTotal /= frameCount;
    
    // Draw frame breakdown averages
    avgFenceWait /= frameCount;
    avgAcquireImage /= frameCount;
    avgEntityData /= frameCount;
    avgCameraUpdate /= frameCount;
    avgRendererDraw /= frameCount;
    avgImguiCmd /= frameCount;
    avgQueueSubmit /= frameCount;
    avgQueuePresent /= frameCount;
    
    _logger->info("=== Frame Timing Results (Average over {} frames) ===", frameCount);
    _logger->info("Poll Events:      {:.3f} ms", avgPollEvents);
    _logger->info("Update Input:     {:.3f} ms", avgUpdateInput);
    _logger->info("Runtime Update:   {:.3f} ms", avgRuntimeUpdate);
    _logger->info("ImGui Draw:       {:.3f} ms", avgImguiDraw);
    _logger->info("Process Input:    {:.3f} ms", avgProcessInput);
    _logger->info("Draw Frame:       {:.3f} ms", avgDrawFrame);
    _logger->info("Total Frame:      {:.3f} ms", avgTotal);
    _logger->info("Average FPS:      {:.1f}", 1000.0 / avgTotal);
    _logger->info("");
    _logger->info("=== Detailed Renderer Timing Breakdown ===");
    _logger->info("Command Buffer Setup:    {:.3f} ms", avgCmdBufferSetup);
    _logger->info("Entity Grouping:         {:.3f} ms", avgEntityGrouping);
    _logger->info("Instance Data Prep:      {:.3f} ms", avgInstanceDataPrep);
    _logger->info("Buffer Updates:          {:.3f} ms", avgBufferUpdates);
    _logger->info("GPU Command Recording:   {:.3f} ms", avgGpuCommandRecording);
    _logger->info("Command Buffer Finish:   {:.3f} ms", avgCmdBufferFinish);
    _logger->info("Renderer Total:          {:.3f} ms", avgRendererTotal);
    _logger->info("");
    _logger->info("=== Application _drawFrame() Breakdown ===");
    _logger->info("Fence Wait:              {:.3f} ms", avgFenceWait);
    _logger->info("Acquire Image:           {:.3f} ms", avgAcquireImage);
    _logger->info("Entity Data Collection:  {:.3f} ms", avgEntityData);
    _logger->info("Camera Update:           {:.3f} ms", avgCameraUpdate);
    _logger->info("Renderer Draw Frame:     {:.3f} ms", avgRendererDraw);
    _logger->info("ImGui Command Buffer:    {:.3f} ms", avgImguiCmd);
    _logger->info("Queue Submit:            {:.3f} ms", avgQueueSubmit);
    _logger->info("Queue Present:           {:.3f} ms", avgQueuePresent);
    _logger->info("");
    _logger->info("Draw Frame Total (sum):  {:.3f} ms", avgFenceWait + avgAcquireImage + avgEntityData + avgCameraUpdate + avgRendererDraw + avgImguiCmd + avgQueueSubmit + avgQueuePresent);
    _logger->info("===============================================");
}
