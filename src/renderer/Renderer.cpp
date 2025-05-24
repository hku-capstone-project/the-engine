#include "Renderer.hpp"
#include "app-context/VulkanApplicationContext.hpp"
#include "config-container/ConfigContainer.hpp"
#include "utils/logger/Logger.hpp"
#include "utils/shader-compiler/ShaderCompiler.hpp"
#include "utils/vulkan-wrapper/memory/Image.hpp"
#include "utils/vulkan-wrapper/memory/Model.hpp"
#include "utils/vulkan-wrapper/pipeline/GfxPipeline.hpp"
#include "window/Window.hpp"

Renderer::Renderer(VulkanApplicationContext *appContext, Logger *logger,
                   ShaderCompiler *shaderCompiler, Window *window, ConfigContainer *configContainer)
    : _appContext(appContext), _logger(logger), _shaderCompiler(shaderCompiler), _window(window),
      _configContainer(configContainer) {
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

    _pipeline = std::make_unique<GfxPipeline>(appContext, logger, glm::vec3(0, 0, 0), nullptr,
                                              shaderCompiler);

    _model            = std::make_unique<Model>(_appContext, _logger,
                                                "./../../../resources/models/sci_sword/sword.gltf");
    _images.baseColor = std::make_unique<Image>(
        _appContext, _logger, "./../../../resources/models/sci_sword/textures/blade_baseColor.png",
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

    _createDepthStencil();
    _createRenderPass();
    _recordTracingCommandBuffers();
    _recordDeliveryCommandBuffers();
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
    attachments[2].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

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

void Renderer::_createDepthStencil() {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.format        = _appContext->getDepthFormat();
    imageInfo.extent        = {_appContext->getSwapchainExtent().width,
                               _appContext->getSwapchainExtent().height, 1};
    imageInfo.mipLevels     = 1;
    imageInfo.arrayLayers   = 1;
    imageInfo.samples       = _appContext->getMsaaSample();
    imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    allocInfo.requiredFlags           = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    VkImage image;
    VmaAllocation allocation;
    vmaCreateImage(_appContext->getAllocator(), &imageInfo, &allocInfo, &image, &allocation,
                   nullptr);
    depthStencil.image = image;

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image                           = depthStencil.image;
    viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format                          = _appContext->getDepthFormat();
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    vkCreateImageView(_appContext->getDevice(), &viewInfo, nullptr, &depthStencil.imageView);
}

void Renderer::_createColorResources() {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.extent        = {_appContext->getSwapchainExtent().width,
                               _appContext->getSwapchainExtent().height, 1};
    imageInfo.mipLevels     = 1;
    imageInfo.arrayLayers   = 1;
    imageInfo.format        = _appContext->getDepthFormat();
    imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    imageInfo.samples     = _appContext->getMsaaSample();
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_CHECK_RESULT(
        vkCreateImage(vulkanDevice->logicalDevice, &imageInfo, nullptr, &colorResources.image));

    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(vulkanDevice->logicalDevice, colorResources.image,
                                 &memoryRequirements);
    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize  = memoryRequirements.size;
    allocateInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memoryRequirements.memoryTypeBits,
                                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(vulkanDevice->logicalDevice, &allocateInfo, nullptr,
                                     &colorResources.memory));
    VK_CHECK_RESULT(vkBindImageMemory(vulkanDevice->logicalDevice, colorResources.image,
                                      colorResources.memory, 0));

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image                           = colorResources.image;
    viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format                          = VK_FORMAT_B8G8R8A8_SRGB;
    viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;

    vkCreateImageView(vulkanDevice->logicalDevice, &viewInfo, nullptr, &colorResources.imageView);
}

void Renderer::_createFrameBuffers() {
    ImageDimensions imgDimensions = _renderTargetImage->getDimensions();

    std::array<VkImageView, 3> attachments = {};

    attachments[0] = colorResources.imageView;
    attachments[1] = depthStencil.imageView;

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

Renderer::~Renderer() { vkDestroyRenderPass(_appContext->getDevice(), _renderPass, nullptr); }

void Renderer::onSwapchainResize() {
    // TODO:
}

void Renderer::drawFrame(size_t currentFrame) {
    ImageDimensions imgDimensions = _renderTargetImage->getDimensions();

    VkCommandBufferBeginInfo cmdBufferBeginInfo{};
    cmdBufferBeginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufferBeginInfo.flags            = 0;
    cmdBufferBeginInfo.pInheritanceInfo = nullptr;

    std::array<VkClearValue, 2> clear_vals = {};
    clear_vals[0].color                    = {{0.025f, 0.025f, 0.025f, 1.0f}};
    clear_vals[1].depthStencil             = {1.0f, 0};

    VkRenderPassBeginInfo rdrPassBeginInfo{};
    rdrPassBeginInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rdrPassBeginInfo.renderPass        = _renderPass;
    rdrPassBeginInfo.renderArea.offset = {0, 0};
    rdrPassBeginInfo.renderArea.extent = _appContext->getSwapchainExtent();
    rdrPassBeginInfo.clearValueCount   = clear_vals.size();
    rdrPassBeginInfo.pClearValues      = clear_vals.data();

    rdrPassBeginInfo.framebuffer = _frameBuffers[currentFrame];
    vkBeginCommandBuffer(_tracingCommandBuffers[currentFrame], &cmdBufferBeginInfo);
    vkCmdBeginRenderPass(_tracingCommandBuffers[currentFrame], &rdrPassBeginInfo,
                         VK_SUBPASS_CONTENTS_INLINE);

    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(_tracingCommandBuffers[currentFrame], 0, 1,
                           &_model->vertexBuffer->getVkBuffer(), offsets);
    vkCmdBindIndexBuffer(_tracingCommandBuffers[currentFrame], _model->indexBuffer->getVkBuffer(),
                         0, VK_INDEX_TYPE_UINT32);

    VkViewport viewport{};
    viewport.width    = imgDimensions.width;
    viewport.height   = imgDimensions.height;
    viewport.maxDepth = 1.0f;
    viewport.minDepth = 0.0f;
    vkCmdSetViewport(_tracingCommandBuffers[currentFrame], 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.extent = {imgDimensions.width, imgDimensions.height};
    scissor.offset = {0, 0};
    vkCmdSetScissor(_tracingCommandBuffers[currentFrame], 0, 1, &scissor);

    vkCmdBindPipeline(_tracingCommandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS,
                      _pipeline->getPipeline());
    // vkCmdBindDescriptorSets(_tracingCommandBuffers[currentFrame],
    // VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline->getPipelineLayout(), 0, 1, &descriptorSet, 0,
    // nullptr);

    vkCmdDrawIndexed(_tracingCommandBuffers[currentFrame], _model->idxCnt, 1, 0, 0, 0);
    vkCmdEndRenderPass(_tracingCommandBuffers[currentFrame]);
    vkEndCommandBuffer(_tracingCommandBuffers[currentFrame]);
}

void Renderer::processInput(double deltaTime) {
    // _camera->processInput(deltaTime);
}

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
}
