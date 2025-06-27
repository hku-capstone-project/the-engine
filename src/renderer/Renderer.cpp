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

#include <filesystem>


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
    std::string modelPath = kPathToResourceFolder + "models/sci_sword/sword.gltf";
    _model = std::make_unique<Model>(_appContext, _logger, modelPath);

    // 提取模型目录，包含 "models/"
    std::string modelDir = modelPath;
    size_t lastSlash = modelDir.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        modelDir = modelDir.substr(0, lastSlash); // 得到 kPathToResourceFolder + "models/sci_sword"
    }
    // 拼接纹理路径，保留 "models/"
    std::string textureBasePath = modelDir + "/textures/"; // 得到 kPathToResourceFolder + "models/sci_sword/textures/"

    auto samplerSettings = Sampler::Settings{
        Sampler::AddressMode::kClampToEdge,
        Sampler::AddressMode::kClampToEdge,
        Sampler::AddressMode::kClampToEdge
    };

    _images.sharedSampler = std::make_unique<Sampler>(_appContext, samplerSettings);

    // 默认纹理路径（通用）
    std::string defaultTexturePath = kPathToResourceFolder + "textures/default_baseColor.png";
    for (const auto &subModel : _model->getModelAttributes().subModels) {
        // 加载 baseColor 贴图
        std::string texturePath = !subModel.baseColorTexturePath.empty()
            ? textureBasePath + subModel.baseColorTexturePath
            : defaultTexturePath;

        if (!subModel.baseColorTexturePath.empty() && !std::filesystem::exists(texturePath)) {
            _logger->error("BaseColor texture file does not exist: {}. Using default.", texturePath);
            texturePath = defaultTexturePath;
        } else if (subModel.baseColorTexturePath.empty()) {
            _logger->warn("SubModel missing BaseColor Texture, using default");
        }

        _images.baseColors.emplace_back(std::make_unique<Image>(
            _appContext, _logger, texturePath,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            _images.sharedSampler->getVkSampler(),
            VK_IMAGE_LAYOUT_UNDEFINED, VK_SAMPLE_COUNT_1_BIT,
            VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT));
        if (_images.baseColors.back()->getVkImage() == VK_NULL_HANDLE) {
            _logger->error("Failed to load BaseColor texture: {}. Using default.", texturePath);
            texturePath = defaultTexturePath;
            _images.baseColors.back() = std::make_unique<Image>(
                _appContext, _logger, texturePath,
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                _images.sharedSampler->getVkSampler(),
                VK_IMAGE_LAYOUT_UNDEFINED, VK_SAMPLE_COUNT_1_BIT,
                VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
            if (_images.baseColors.back()->getVkImage() == VK_NULL_HANDLE) {
                _logger->error("Failed to load default BaseColor texture: {}. Aborting.", texturePath);
                exit(1);
            }
        } else {
            _logger->info("Loaded BaseColor Texture: {}", texturePath);
        }

        // 加载 emissive 贴图
        texturePath = !subModel.emissiveTexturePath.empty()
            ? textureBasePath + subModel.emissiveTexturePath
            : "";
        if (!subModel.emissiveTexturePath.empty()) {
            if (!std::filesystem::exists(texturePath)) {
                _logger->warn("Emissive texture file does not exist: {}. Skipping.", texturePath);
                _images.emissiveTextures.emplace_back(nullptr);
            } else {
                _images.emissiveTextures.emplace_back(std::make_unique<Image>(
                    _appContext, _logger, texturePath,
                    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                    _images.sharedSampler->getVkSampler(),
                    VK_IMAGE_LAYOUT_UNDEFINED, VK_SAMPLE_COUNT_1_BIT,
                    VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT));
                if (_images.emissiveTextures.back()->getVkImage() == VK_NULL_HANDLE) {
                    _logger->warn("Failed to load Emissive texture: {}. Skipping.", texturePath);
                    _images.emissiveTextures.back() = nullptr;
                } else {
                    _logger->info("Loaded Emissive Texture: {}", texturePath);
                }
            }
        } else {
            _logger->warn("SubModel missing Emissive Texture, skipping");
            _images.emissiveTextures.emplace_back(nullptr);
        }

        // 加载 metallicRoughness 贴图
        texturePath = !subModel.metallicRoughnessTexturePath.empty()
            ? textureBasePath + subModel.metallicRoughnessTexturePath
            : "";
        if (!subModel.metallicRoughnessTexturePath.empty()) {
            if (!std::filesystem::exists(texturePath)) {
                _logger->warn("MetallicRoughness texture file does not exist: {}. Skipping.", texturePath);
                _images.metallicRoughnessTextures.emplace_back(nullptr);
            } else {
                _images.metallicRoughnessTextures.emplace_back(std::make_unique<Image>(
                    _appContext, _logger, texturePath,
                    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                    _images.sharedSampler->getVkSampler(),
                    VK_IMAGE_LAYOUT_UNDEFINED, VK_SAMPLE_COUNT_1_BIT,
                    VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT));
                if (_images.metallicRoughnessTextures.back()->getVkImage() == VK_NULL_HANDLE) {
                    _logger->warn("Failed to load MetallicRoughness texture: {}. Skipping.", texturePath);
                    _images.metallicRoughnessTextures.back() = nullptr;
                } else {
                    _logger->info("Loaded MetallicRoughness Texture: {}", texturePath);
                }
            }
        } else {
            _logger->warn("SubModel missing MetallicRoughness Texture, skipping");
            _images.metallicRoughnessTextures.emplace_back(nullptr);
        }

        // 加载 normal 贴图
        texturePath = !subModel.normalTexturePath.empty()
            ? textureBasePath + subModel.normalTexturePath
            : "";
        if (!subModel.normalTexturePath.empty()) {
            if (!std::filesystem::exists(texturePath)) {
                _logger->warn("Normal texture file does not exist: {}. Skipping.", texturePath);
                _images.normalTextures.emplace_back(nullptr);
            } else {
                _images.normalTextures.emplace_back(std::make_unique<Image>(
                    _appContext, _logger, texturePath,
                    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                    _images.sharedSampler->getVkSampler(),
                    VK_IMAGE_LAYOUT_UNDEFINED, VK_SAMPLE_COUNT_1_BIT,
                    VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT));
                if (_images.normalTextures.back()->getVkImage() == VK_NULL_HANDLE) {
                    _logger->warn("Failed to load Normal texture: {}. Skipping.", texturePath);
                    _images.normalTextures.back() = nullptr;
                } else {
                    _logger->info("Loaded Normal Texture: {}", texturePath);
                }
            }
        } else {
            _logger->warn("SubModel missing Normal Texture, skipping");
            _images.normalTextures.emplace_back(nullptr);
        }
    }

    _createBuffersAndBufferBundles();
    _createDescriptorSetBundle();
    _createRenderPass();
    _createGraphicsPipeline();
    _createDepthStencil();
    _createColorResources();
    _createFrameBuffers();
    _recordDrawingCommandBuffers();
    _recordDeliveryCommandBuffers();

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
    
    // 绑定所有贴图到不同绑定点
    for (size_t i = 0; i < _images.baseColors.size(); ++i) {
        _descriptorSetBundle->bindImageSampler(1 + i * 4, _images.baseColors[i].get()); // binding 1, 5, 9...
        if (_images.emissiveTextures[i]) {
            _descriptorSetBundle->bindImageSampler(2 + i * 4, _images.emissiveTextures[i].get()); // binding 2, 6, 10...
        }
        if (_images.metallicRoughnessTextures[i]) {
            _descriptorSetBundle->bindImageSampler(3 + i * 4, _images.metallicRoughnessTextures[i].get()); // binding 3, 7, 11...
        }
        if (_images.normalTextures[i]) {
            _descriptorSetBundle->bindImageSampler(4 + i * 4, _images.normalTextures[i].get()); // binding 4, 8, 12...
        }
    }
    _descriptorSetBundle->create();
}

// 以下保持不变
void Renderer::_createRenderPass() {
    std::array<VkAttachmentDescription, 3> attachments = {};
    attachments[0].format         = VK_FORMAT_B8G8R8A8_UNORM;
    attachments[0].samples        = _appContext->getMsaaSample();
    attachments[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[1].format         = _appContext->getDepthFormat();
    attachments[1].samples        = _appContext->getMsaaSample();
    attachments[1].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments[2].format         = VK_FORMAT_B8G8R8A8_UNORM;
    attachments[2].samples        = VK_SAMPLE_COUNT_1_BIT;
    attachments[2].loadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[2].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[2].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[2].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[2].finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentResolveRef{};
    colorAttachmentResolveRef.attachment = 2;
    colorAttachmentResolveRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription description{};
    description.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    description.colorAttachmentCount    = 1;
    description.pColorAttachments       = &colorAttachmentRef;
    description.pDepthStencilAttachment = &depthAttachmentRef;
    description.pResolveAttachments     = &colorAttachmentResolveRef;

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
    ImageDimensions dim{_appContext->getSwapchainExtent().width,
                        _appContext->getSwapchainExtent().height, 1};

    _depthStencilImage = std::make_unique<Image>(
        _appContext, _logger, dim, _appContext->getDepthFormat(),
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_NULL_HANDLE,
        VK_IMAGE_LAYOUT_UNDEFINED, _appContext->getMsaaSample(),
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
}

void Renderer::_createColorResources() {
    ImageDimensions dim{_appContext->getSwapchainExtent().width,
                        _appContext->getSwapchainExtent().height, 1};

    _colorResourcesImage = std::make_unique<Image>(
        _appContext, _logger, dim, VK_FORMAT_B8G8R8A8_UNORM,
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_NULL_HANDLE,
        VK_IMAGE_LAYOUT_UNDEFINED, _appContext->getMsaaSample(),
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
    if (!_drawingCommandBuffers.empty()) {
        vkFreeCommandBuffers(_appContext->getDevice(), _appContext->getCommandPool(),
                             static_cast<uint32_t>(_drawingCommandBuffers.size()),
                             _drawingCommandBuffers.data());
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
    _logger->info("Swapchain has been resized. Recreating dependent resources.");

    // 1. Clean up resources that depend on the swapchain's resolution or images.
    // ---------------------------------------------------------------------------

    // Destroy existing framebuffers
    for (auto framebuffer : _frameBuffers) {
        vkDestroyFramebuffer(_appContext->getDevice(), framebuffer, nullptr);
    }
    _frameBuffers.clear();

    // Reset unique_ptrs for images, which will trigger their destructors
    _colorResourcesImage.reset();
    _depthStencilImage.reset();
    _renderTargetImage.reset();

    // Note: Command buffers are implicitly handled by the _record... functions,
    // which free existing buffers before re-allocating them.

    // 2. Recreate the resources with the new swapchain properties.
    // ---------------------------------------------------------------------------

    // Recreate the main render target image with the new dimensions
    VkExtent2D newExtent = _appContext->getSwapchainExtent();
    _renderTargetImage   = std::make_unique<Image>(
        _appContext, _logger, ImageDimensions{newExtent.width, newExtent.height},
        _appContext->getSwapchainImageFormat(),
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

    // Recreate resolution-dependent resources by calling their creation functions
    _createDepthStencil();
    _createColorResources();
    _createFrameBuffers();

    // Re-allocate and re-record command buffers
    _recordDrawingCommandBuffers();
    _recordDeliveryCommandBuffers();

    _logger->info("Renderer resources have been successfully recreated.");
}

void Renderer::_updateBufferData(size_t currentFrame, glm::mat4 modelMatrix) {
    auto view = _camera->getViewMatrix();
    auto swapchainExtent = _appContext->getSwapchainExtent();
    auto proj = _camera->getProjectionMatrix(static_cast<float>(swapchainExtent.width) /
                                             static_cast<float>(swapchainExtent.height));
    auto model = glm::mat4(1.0f);

    // 假设 _camera 有 getPosition 方法
    auto viewPos = _camera->getPosition(); // 获取摄像机位置

    S_RenderInfo renderInfo{};
    renderInfo.view = view;
    renderInfo.proj = proj;
    renderInfo.model = model;
    renderInfo.viewPos = viewPos; // 假设 S_RenderInfo 有 viewPos 字段

    _renderInfoBufferBundle->getBuffer(currentFrame)->fillData(&renderInfo);
}

void Renderer::drawFrame(size_t currentFrame, size_t imageIndex, glm::mat4 modelMatrix) {
    _updateBufferData(currentFrame, modelMatrix);

    auto &cmdBuffer = _tracingCommandBuffers[currentFrame];
    VkExtent2D currentSwapchainExtent = _appContext->getSwapchainExtent();

    VkCommandBufferBeginInfo cmdBufferBeginInfo{};
    cmdBufferBeginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufferBeginInfo.flags            = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    cmdBufferBeginInfo.pInheritanceInfo = nullptr;

    std::array<VkClearValue, 2> clear_vals = {};
    clear_vals[0].color        = {{0.1f, 0.1f, 0.1f, 1.0f}};
    clear_vals[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo rdrPassBeginInfo{};
    rdrPassBeginInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rdrPassBeginInfo.renderPass        = _renderPass;
    rdrPassBeginInfo.renderArea.offset = {0, 0};
    rdrPassBeginInfo.renderArea.extent = _appContext->getSwapchainExtent();
    rdrPassBeginInfo.clearValueCount   = clear_vals.size();
    rdrPassBeginInfo.pClearValues      = clear_vals.data();
    rdrPassBeginInfo.framebuffer       = _frameBuffers[imageIndex];

    vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo);

    // 转换所有贴图的布局
    std::vector<VkImageMemoryBarrier> barriers;
    for (const auto &baseColor : _images.baseColors) {
        VkImageMemoryBarrier barrier{};
        barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout                       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.image                           = baseColor->getVkImage();
        barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = 1;
        barrier.srcAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask                   = VK_ACCESS_SHADER_READ_BIT;
        barriers.push_back(barrier);
    }
    for (const auto &emissive : _images.emissiveTextures) {
        if (emissive) {
            VkImageMemoryBarrier barrier{};
            barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout                       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barrier.image                           = emissive->getVkImage();
            barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel   = 0;
            barrier.subresourceRange.levelCount     = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount     = 1;
            barrier.srcAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask                   = VK_ACCESS_SHADER_READ_BIT;
            barriers.push_back(barrier);
        }
    }
    for (const auto &metallicRoughness : _images.metallicRoughnessTextures) {
        if (metallicRoughness) {
            VkImageMemoryBarrier barrier{};
            barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout                       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barrier.image                           = metallicRoughness->getVkImage();
            barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel   = 0;
            barrier.subresourceRange.levelCount     = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount     = 1;
            barrier.srcAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask                   = VK_ACCESS_SHADER_READ_BIT;
            barriers.push_back(barrier);
        }
    }
    for (const auto &normal : _images.normalTextures) {
        if (normal) {
            VkImageMemoryBarrier barrier{};
            barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout                       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barrier.image                           = normal->getVkImage();
            barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel   = 0;
            barrier.subresourceRange.levelCount     = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount     = 1;
            barrier.srcAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask                   = VK_ACCESS_SHADER_READ_BIT;
            barriers.push_back(barrier);
        }
    }

    if (!barriers.empty()) {
        vkCmdPipelineBarrier(cmdBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr,
                             static_cast<uint32_t>(barriers.size()), barriers.data());
    }

    vkCmdBeginRenderPass(cmdBuffer, &rdrPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

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

    for (size_t i = 0; i < _model->subModelBuffers.size(); ++i) {
        const auto &subModel = _model->subModelBuffers[i];
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &subModel.vertexBuffer->getVkBuffer(), offsets);
        vkCmdBindIndexBuffer(cmdBuffer, subModel.indexBuffer->getVkBuffer(), 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                _pipeline->getPipelineLayout(), 0, 1,
                                &_descriptorSetBundle->getDescriptorSet(currentFrame), 0, nullptr);

        uint32_t textureIndex = static_cast<uint32_t>(i);
        vkCmdPushConstants(cmdBuffer, _pipeline->getPipelineLayout(),
                           VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t), &textureIndex);

        vkCmdDrawIndexed(cmdBuffer, subModel.idxCnt, 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(cmdBuffer);

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

void Renderer::_recordDrawingCommandBuffers() {
    for (auto &commandBuffer : _drawingCommandBuffers) {
        vkFreeCommandBuffers(_appContext->getDevice(), _appContext->getCommandPool(), 1,
                             &commandBuffer);
    }
    _drawingCommandBuffers.clear();

    _drawingCommandBuffers.resize(_appContext->getSwapchainImagesCount());
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool        = _appContext->getCommandPool();
    allocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = static_cast<uint32_t>(_drawingCommandBuffers.size());
    vkAllocateCommandBuffers(_appContext->getDevice(), &allocateInfo,
                             _drawingCommandBuffers.data());
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

    for (size_t i = 0; i < _deliveryCommandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        vkBeginCommandBuffer(_deliveryCommandBuffers[i], &beginInfo);

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