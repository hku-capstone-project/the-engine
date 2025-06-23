#include "Renderer.hpp"
#include "ShaderSharedVariables.hpp"
#include "app-context/VulkanApplicationContext.hpp"
#include "camera/Camera.hpp"
#include "config-container/ConfigContainer.hpp"
#include "config/RootDir.h"
#include "utils/logger/Logger.hpp"
#include "utils/shader-compiler/ShaderCompiler.hpp"
#include "utils/vulkan-wrapper/descriptor-set/DescriptorSetBundle.hpp"
#include "utils/vulkan-wrapper/memory/Buffer.hpp"
#include "utils/vulkan-wrapper/memory/BufferBundle.hpp"
#include "utils/vulkan-wrapper/memory/Image.hpp"
#include "utils/vulkan-wrapper/memory/Model.hpp"
#include "utils/vulkan-wrapper/pipeline/GfxPipeline.hpp"
#include "utils/vulkan-wrapper/sampler/Sampler.hpp"
#include "window/Window.hpp"

Renderer::Renderer(VulkanApplicationContext *appContext, Logger *logger, size_t framesInFlight,
                   ShaderCompiler *shaderCompiler, Window *window, ConfigContainer *configContainer)
    : _appContext(appContext), _logger(logger), _framesInFlight(framesInFlight),
      _shaderCompiler(shaderCompiler), _window(window), _configContainer(configContainer) {
    _camera = std::make_unique<Camera>(_window, logger, configContainer);

    int width  = 0;
    int height = 0;
    window->getWindowDimension(width, height);
    logger->info("Window dimension: {} x {}", width, height);

    _renderTargetImage = std::make_unique<Image>(
        _appContext, logger,
        ImageDimensions{static_cast<uint32_t>(width), static_cast<uint32_t>(height)},
        _appContext->getSwapchainImageFormat(),
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

    const auto testModelPath = kPathToResourceFolder + "models/blender-monkey/monkey.obj";
    // const auto testModelPath = kPathToResourceFolder + "models/sci_sword/sword.gltf";
    _model = std::make_unique<Model>(_appContext, _logger, testModelPath);

    auto samplerSettings = Sampler::Settings{
        Sampler::AddressMode::kClampToEdge, // U
        Sampler::AddressMode::kClampToEdge, // V
        Sampler::AddressMode::kClampToEdge  // W
    };

    _images.sharedSampler = std::make_unique<Sampler>(_appContext, samplerSettings);
    _images.baseColor     = std::make_unique<Image>(
        _appContext, _logger,
        kPathToResourceFolder + "models/sci_sword/textures/blade_baseColor.png",
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        _images.sharedSampler->getVkSampler());

    _createBuffersAndBufferBundles();
    _createDescriptorSetBundle();
    _createRenderPass();
    _createGraphicsPipeline();

    _createDepthStencil();
    _createColorResources();
    _createFrameBuffers();
    _recordTracingCommandBuffers();
    _recordDeliveryCommandBuffers();

    // attach camera's mouse handler to the window mouse callback
    _window->addCursorMoveCallback(
        [this](CursorMoveInfo const &mouseInfo) { _camera->handleMouseMovement(mouseInfo); });
}

void Renderer::_createBuffersAndBufferBundles() {
    _renderInfoBufferBundle = std::make_unique<BufferBundle>(
        _appContext, _framesInFlight, sizeof(S_RenderInfo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        MemoryStyle::kHostVisible);
}

void Renderer::_createDescriptorSetBundle() {
    _descriptorSetBundle = std::make_unique<DescriptorSetBundle>(
        _appContext, _framesInFlight, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    _descriptorSetBundle->bindUniformBufferBundle(0, _renderInfoBufferBundle.get());
    _descriptorSetBundle->bindImageSampler(1, _images.baseColor.get());
    _descriptorSetBundle->create();
}

void Renderer::_createRenderPass() {
    std::array<VkAttachmentDescription, 3> attachments = {};
    // Color attachment
    attachments[0].format         = VK_FORMAT_B8G8R8A8_UNORM;
    attachments[0].samples        = _appContext->getMsaaSample();
    attachments[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // Depth attachment
    attachments[1].format         = _appContext->getDepthFormat();
    attachments[1].samples        = _appContext->getMsaaSample();
    attachments[1].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    // Resolve attachment
    attachments[2].format         = VK_FORMAT_B8G8R8A8_UNORM;
    attachments[2].samples        = VK_SAMPLE_COUNT_1_BIT;
    attachments[2].loadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[2].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[2].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[2].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[2].finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Attachment Reference
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentResolveRef{};
    colorAttachmentResolveRef.attachment = 2;
    colorAttachmentResolveRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // SubPass Description
    VkSubpassDescription description{};
    description.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    description.colorAttachmentCount    = 1;
    description.pColorAttachments       = &colorAttachmentRef;
    description.pDepthStencilAttachment = &depthAttachmentRef;
    description.pResolveAttachments     = &colorAttachmentResolveRef;

    // SubPass Dependency
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    // Render Pass
    VkRenderPassCreateInfo createInfo{};
    createInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    createInfo.pAttachments    = attachments.data();
    createInfo.subpassCount    = 1;
    createInfo.pSubpasses      = &description;
    createInfo.dependencyCount = 1;
    createInfo.pDependencies   = &dependency;
    vkCreateRenderPass(_appContext->getDevice(), &createInfo, nullptr, &_renderPass);
}

void Renderer::_createGraphicsPipeline() {
    _pipeline = std::make_unique<GfxPipeline>(
        _appContext, _logger, kPathToResourceFolder + "shaders/default", _descriptorSetBundle.get(),
        _shaderCompiler, _renderPass);
}

void Renderer::_createDepthStencil() {
    // build a small ImageDimensions struct
    ImageDimensions dim{_appContext->getSwapchainExtent().width,
                        _appContext->getSwapchainExtent().height, 1};

    _depthStencilImage = std::make_unique<Image>(
        _appContext, _logger, dim, _appContext->getDepthFormat(),
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        /* no sampler needed */ VK_NULL_HANDLE,
        /* initial layout */ VK_IMAGE_LAYOUT_UNDEFINED, _appContext->getMsaaSample(),
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
}

void Renderer::_createColorResources() {
    // build dimensions
    ImageDimensions dim{_appContext->getSwapchainExtent().width,
                        _appContext->getSwapchainExtent().height, 1};

    // Note: we pass both TRANSIENT_ATTACHMENT_BIT and COLOR_ATTACHMENT_BIT,
    // no sampler is needed, and we transition straight to COLOR_ATTACHMENT_OPTIMAL:
    _colorResourcesImage = std::make_unique<Image>(
        _appContext, _logger, dim, VK_FORMAT_B8G8R8A8_UNORM,
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        /* sampler */ VK_NULL_HANDLE,
        /* initialLayout */ VK_IMAGE_LAYOUT_UNDEFINED, _appContext->getMsaaSample(),
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
}

void Renderer::_createFrameBuffers() {
    ImageDimensions imgDimensions = _renderTargetImage->getDimensions();

    std::array<VkImageView, 3> attachments = {};

    attachments[0] = _colorResourcesImage->getVkImageView();
    attachments[1] = _depthStencilImage->getVkImageView();

    VkFramebufferCreateInfo createInfo{};
    createInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    createInfo.width           = imgDimensions.width;
    createInfo.height          = imgDimensions.height;
    createInfo.pNext           = nullptr;
    createInfo.renderPass      = _renderPass;
    createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    createInfo.pAttachments    = attachments.data();
    createInfo.layers          = 1;

    _frameBuffers.resize(_appContext->getSwapchainImagesCount());
    const std::vector<VkImageView> &swapchainViews = _appContext->getSwapchainImageViews();
    for (uint32_t i = 0; i < _frameBuffers.size(); i++) {
        attachments[2] = swapchainViews[i];
        vkCreateFramebuffer(_appContext->getDevice(), &createInfo, nullptr, &_frameBuffers[i]);
    }
}

Renderer::~Renderer() {
    for (auto framebuffer : _frameBuffers) {
        vkDestroyFramebuffer(_appContext->getDevice(), framebuffer, nullptr);
    }

    if (!_tracingCommandBuffers.empty()) {
        vkFreeCommandBuffers(_appContext->getDevice(), _appContext->getCommandPool(),
                             static_cast<uint32_t>(_tracingCommandBuffers.size()),
                             _tracingCommandBuffers.data());
    }
    if (!_deliveryCommandBuffers.empty()) {
        vkFreeCommandBuffers(_appContext->getDevice(), _appContext->getCommandPool(),
                             static_cast<uint32_t>(_deliveryCommandBuffers.size()),
                             _deliveryCommandBuffers.data());
    }

    _pipeline.reset();

    vkDestroyRenderPass(_appContext->getDevice(), _renderPass, nullptr);
}

void Renderer::onSwapchainResize() {
    // TODO:
}

void Renderer::_updateUboData(size_t currentFrame) {
    auto view = _camera->getViewMatrix();

    auto swapchainExtent = _appContext->getSwapchainExtent();
    auto proj            = _camera->getProjectionMatrix(static_cast<float>(swapchainExtent.width) /
                                                        static_cast<float>(swapchainExtent.height));

    auto model = glm::mat4(1.0f);

    S_RenderInfo renderInfo{};
    renderInfo.view  = view;
    renderInfo.proj  = proj;
    renderInfo.model = model;

    _renderInfoBufferBundle->getBuffer(currentFrame)->fillData(&renderInfo);
}

void Renderer::drawFrame(size_t currentFrame, size_t imageIndex) {
    _updateUboData(currentFrame);

    auto &cmdBuffer = _tracingCommandBuffers[currentFrame];

    // ImageDimensions imgDimensions = _renderTargetImage->getDimensions();
    VkExtent2D currentSwapchainExtent = _appContext->getSwapchainExtent();

    VkCommandBufferBeginInfo cmdBufferBeginInfo{};
    cmdBufferBeginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufferBeginInfo.flags            = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    cmdBufferBeginInfo.pInheritanceInfo = nullptr;

    std::array<VkClearValue, 2> clear_vals = {};
    clear_vals[0].color        = {{0.1f, 0.1f, 0.1f, 1.0f}}; // 稍微亮一点的背景色
    clear_vals[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo rdrPassBeginInfo{};
    rdrPassBeginInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rdrPassBeginInfo.renderPass        = _renderPass;
    rdrPassBeginInfo.renderArea.offset = {0, 0};
    rdrPassBeginInfo.renderArea.extent = _appContext->getSwapchainExtent();
    rdrPassBeginInfo.clearValueCount   = clear_vals.size();
    rdrPassBeginInfo.pClearValues      = clear_vals.data();

    rdrPassBeginInfo.framebuffer = _frameBuffers[imageIndex];

    vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo);
    vkCmdBeginRenderPass(cmdBuffer, &rdrPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &_model->vertexBuffer->getVkBuffer(), offsets);
    vkCmdBindIndexBuffer(cmdBuffer, _model->indexBuffer->getVkBuffer(), 0, VK_INDEX_TYPE_UINT32);

    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = static_cast<float>(currentSwapchainExtent.width);
    viewport.height   = static_cast<float>(currentSwapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = currentSwapchainExtent;
    vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline->getPipeline());

    // TODO:
    _pipeline->recordDrawIndexed(cmdBuffer, currentFrame);
    vkCmdDrawIndexed(cmdBuffer, _model->idxCnt, 1, 0, 0, 0);
    vkCmdEndRenderPass(cmdBuffer);

    // 添加渲染完成后的图像布局转换
    VkImageMemoryBarrier barrier{};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout                       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.newLayout                       = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = _appContext->getSwapchainImages()[imageIndex];
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;
    barrier.srcAccessMask                   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask                   = 0;

    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &barrier);

    vkEndCommandBuffer(cmdBuffer);
}

void Renderer::processInput(double deltaTime) { _camera->processInput(deltaTime); }

void Renderer::_recordTracingCommandBuffers() {
    for (auto &commandBuffer : _tracingCommandBuffers) {
        vkFreeCommandBuffers(_appContext->getDevice(), _appContext->getCommandPool(), 1,
                             &commandBuffer);
    }
    _tracingCommandBuffers.clear();

    _tracingCommandBuffers.resize(_appContext->getSwapchainImagesCount());
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool        = _appContext->getCommandPool();
    allocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = static_cast<uint32_t>(_tracingCommandBuffers.size());
    vkAllocateCommandBuffers(_appContext->getDevice(), &allocateInfo,
                             _tracingCommandBuffers.data());
}

void Renderer::_recordDeliveryCommandBuffers() {
    for (auto &commandBuffer : _deliveryCommandBuffers) {
        vkFreeCommandBuffers(_appContext->getDevice(), _appContext->getCommandPool(), 1,
                             &commandBuffer);
    }
    _deliveryCommandBuffers.clear();

    _deliveryCommandBuffers.resize(_appContext->getSwapchainImagesCount());

    VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandPool        = _appContext->getCommandPool();
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(_deliveryCommandBuffers.size());

    vkAllocateCommandBuffers(_appContext->getDevice(), &allocInfo, _deliveryCommandBuffers.data());

    // 记录命令
    for (size_t i = 0; i < _deliveryCommandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        vkBeginCommandBuffer(_deliveryCommandBuffers[i], &beginInfo);

        // 转换图像布局
        VkImageMemoryBarrier barrier{};
        barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout                       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.image                           = _appContext->getSwapchainImages()[i];
        barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = 1;
        barrier.srcAccessMask                   = 0;
        barrier.dstAccessMask                   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        vkCmdPipelineBarrier(_deliveryCommandBuffers[i], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0,
                             nullptr, 1, &barrier);

        vkEndCommandBuffer(_deliveryCommandBuffers[i]);
    }
}
