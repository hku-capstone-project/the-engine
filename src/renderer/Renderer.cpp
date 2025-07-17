#include "Renderer.hpp"
#include "ShaderSharedVariables.hpp"
#include "app-context/VulkanApplicationContext.hpp"
#include "camera/Camera.hpp"
#include "config-container/ConfigContainer.hpp"
#include "config/RootDir.h"
#include "dotnet/Components.hpp"
#include "dotnet/RuntimeApplication.hpp"
#include "dotnet/RuntimeBridge.hpp"
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

#include <map>

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

    // Load models from RuntimeApplication mesh registry
    auto meshes = RuntimeBridge::getRuntimeApplication().getAllMeshes();
    _logger->info("Loading {} meshes from C# registry", meshes.size());

    if (meshes.empty()) {
        _logger->warn("No meshes registered! Renderer will be created with empty mesh list.");
        _logger->warn("This may cause rendering issues. Ensure mesh registration happens before "
                      "Renderer creation.");
    } else {
        for (const auto &[meshId, meshPath] : meshes) {
            std::string fullPath = kPathToResourceFolder + meshPath;
            _models.push_back(std::make_unique<Model>(_appContext, _logger, fullPath));
            _logger->info("Loaded mesh ID {}: {}", meshId, fullPath);
        }
    }

    _createDefaultTextures();
    _createModelImages();
    _createBuffersAndBufferBundles();
    _createDescriptorSetBundles();
    _createRenderPass();
    _createGraphicsPipeline();

    _createDepthStencil();
    _createColorResources();
    _createFrameBuffers();
    _recordDrawingCommandBuffers();
    _recordDeliveryCommandBuffers();

    // attach camera's mouse handler to the window mouse callback
    _window->addCursorMoveCallback(
        [this](CursorMoveInfo const &mouseInfo) { _camera->handleMouseMovement(mouseInfo); });
}

void Renderer::_createModelImages() {
    _modelImages.clear();

    if (_models.empty()) {
        _logger->warn("No models loaded, skipping model image creation");
        return;
    }

    auto samplerSettings = Sampler::Settings{
        Sampler::AddressMode::kClampToEdge, // U
        Sampler::AddressMode::kClampToEdge, // V
        Sampler::AddressMode::kClampToEdge  // W
    };

    for (size_t i = 0; i < _models.size(); ++i) {
        std::vector<ModelImages> modelMeshesImages;
        auto &model = *_models[i];

        for (size_t j = 0; j < model.baseColorTexturePaths.size(); ++j) {
            ModelImages images;
            images.sharedSampler = std::make_unique<Sampler>(_appContext, samplerSettings);

            // baseColor
            const std::string &path = model.baseColorTexturePaths[j];
            if (!path.empty()) {
                auto tempImage = std::make_unique<Image>(_appContext, _logger, path,
                                                         VK_IMAGE_USAGE_SAMPLED_BIT |
                                                             VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                                         images.sharedSampler->getVkSampler());
                if (tempImage->getVkImage() != VK_NULL_HANDLE) {
                    images.baseColor = std::move(tempImage);
                } else {
                    _logger->warn("Failed to load baseColor texture: {}, using default", path);
                    images.baseColor = std::make_unique<Image>(
                        _appContext, _logger, ImageDimensions{1, 1, 1}, VK_FORMAT_R8G8B8A8_SRGB,
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        images.sharedSampler->getVkSampler());
                    std::array<uint8_t, 4> pixel = {255, 255, 255, 255};
                    _uploadTextureData(images.baseColor.get(), pixel.data());
                }
            } else {
                images.baseColor = std::make_unique<Image>(
                    _appContext, _logger, ImageDimensions{1, 1, 1}, VK_FORMAT_R8G8B8A8_SRGB,
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    images.sharedSampler->getVkSampler());
                std::array<uint8_t, 4> pixel = {255, 255, 255, 255};
                _uploadTextureData(images.baseColor.get(), pixel.data());
            }

            // normalMap
            const std::string &normalPath = model.normalTexturePaths[j];
            if (!normalPath.empty()) {
                auto tempImage = std::make_unique<Image>(_appContext, _logger, normalPath,
                                                         VK_IMAGE_USAGE_SAMPLED_BIT |
                                                             VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                                         images.sharedSampler->getVkSampler());
                if (tempImage->getVkImage() != VK_NULL_HANDLE) {
                    images.normalMap = std::move(tempImage);
                } else {
                    _logger->warn("Failed to load normal texture: {}, using default", normalPath);
                    images.normalMap = std::make_unique<Image>(
                        _appContext, _logger, ImageDimensions{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM,
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        images.sharedSampler->getVkSampler());
                    std::array<uint8_t, 4> pixel = {128, 128, 255, 255};
                    _uploadTextureData(images.normalMap.get(), pixel.data());
                }
            } else {
                images.normalMap = std::make_unique<Image>(
                    _appContext, _logger, ImageDimensions{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM,
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    images.sharedSampler->getVkSampler());
                std::array<uint8_t, 4> pixel = {128, 128, 255, 255};
                _uploadTextureData(images.normalMap.get(), pixel.data());
            }

            // metalRoughness
            const std::string &mrPath = model.metallicRoughnessTexturePaths[j];
            if (!mrPath.empty()) {
                auto tempImage = std::make_unique<Image>(_appContext, _logger, mrPath,
                                                         VK_IMAGE_USAGE_SAMPLED_BIT |
                                                             VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                                         images.sharedSampler->getVkSampler());
                if (tempImage->getVkImage() != VK_NULL_HANDLE) {
                    images.metalRoughness = std::move(tempImage);
                } else {
                    _logger->warn("Failed to load metalRoughness texture: {}, using default",
                                  mrPath);
                    images.metalRoughness = std::make_unique<Image>(
                        _appContext, _logger, ImageDimensions{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM,
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        images.sharedSampler->getVkSampler());
                    std::array<uint8_t, 4> pixel = {0, 255, 0, 255};
                    _uploadTextureData(images.metalRoughness.get(), pixel.data());
                }
            } else {
                images.metalRoughness = std::make_unique<Image>(
                    _appContext, _logger, ImageDimensions{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM,
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    images.sharedSampler->getVkSampler());
                std::array<uint8_t, 4> pixel = {0, 255, 0, 255};
                _uploadTextureData(images.metalRoughness.get(), pixel.data());
            }

            // emissive
            const std::string &emPath = model.emissiveTexturePaths[j];
            if (!emPath.empty()) {
                auto tempImage = std::make_unique<Image>(_appContext, _logger, emPath,
                                                         VK_IMAGE_USAGE_SAMPLED_BIT |
                                                             VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                                         images.sharedSampler->getVkSampler());
                if (tempImage->getVkImage() != VK_NULL_HANDLE) {
                    images.emissive = std::move(tempImage);
                } else {
                    _logger->warn("Failed to load emissive texture: {}, using default", emPath);
                    images.emissive = std::make_unique<Image>(
                        _appContext, _logger, ImageDimensions{1, 1, 1}, VK_FORMAT_R8G8B8A8_SRGB,
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        images.sharedSampler->getVkSampler());
                    std::array<uint8_t, 4> pixel = {0, 0, 0, 255};
                    _uploadTextureData(images.emissive.get(), pixel.data());
                }
            } else {
                images.emissive = std::make_unique<Image>(
                    _appContext, _logger, ImageDimensions{1, 1, 1}, VK_FORMAT_R8G8B8A8_SRGB,
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    images.sharedSampler->getVkSampler());
                std::array<uint8_t, 4> pixel = {0, 0, 0, 255};
                _uploadTextureData(images.emissive.get(), pixel.data());
            }

            modelMeshesImages.push_back(std::move(images));
        }
        _modelImages.push_back(std::move(modelMeshesImages));
    }
}

void Renderer::_createBuffersAndBufferBundles() {
    _renderInfoBufferBundles.clear();
    _materialBufferBundles.clear();
    _instanceBufferBundles.clear();

    if (_models.empty()) {
        _logger->warn("No models loaded, skipping buffer bundle creation");
        return;
    }

    // create one buffer bundle per model
    for (size_t i = 0; i < _models.size(); ++i) {
        // S_RenderInfo 缓冲区
        _renderInfoBufferBundles.push_back(std::make_unique<BufferBundle>(
            _appContext, _framesInFlight, sizeof(S_RenderInfo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            MemoryStyle::kHostVisible));

        // MaterialUBO 缓冲区
        _materialBufferBundles.push_back(std::make_unique<BufferBundle>(
            _appContext, _framesInFlight, sizeof(S_MaterialInfo),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, MemoryStyle::kHostVisible));

        // Instance buffer for model matrices (assumes max 1000 instances per model)
        const size_t MAX_INSTANCES = 1000;
        _instanceBufferBundles.push_back(std::make_unique<BufferBundle>(
            _appContext, _framesInFlight, sizeof(glm::mat4) * MAX_INSTANCES,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, MemoryStyle::kHostVisible));
    }
}

void Renderer::_createDescriptorSetBundles() {
    _descriptorSetBundles.clear();

    if (_models.empty()) {
        _logger->warn("No models loaded, skipping descriptor set bundle creation");
        return;
    }

    for (size_t i = 0; i < _models.size(); ++i) {
        std::vector<std::unique_ptr<DescriptorSetBundle>> modelDescBundles;
        for (size_t j = 0; j < _modelImages[i].size(); ++j) {
            auto descBundle = std::make_unique<DescriptorSetBundle>(
                _appContext, _framesInFlight,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
            descBundle->bindUniformBufferBundle(0, _renderInfoBufferBundles[i].get());
            descBundle->bindUniformBufferBundle(2, _materialBufferBundles[i].get());

            descBundle->bindImageSampler(1, _modelImages[i][j].baseColor.get());
            descBundle->bindImageSampler(3, _modelImages[i][j].normalMap.get());
            descBundle->bindImageSampler(4, _modelImages[i][j].metalRoughness.get());
            descBundle->bindImageSampler(5, _modelImages[i][j].emissive.get());

            descBundle->create();
            modelDescBundles.push_back(std::move(descBundle));
        }
        _descriptorSetBundles.push_back(std::move(modelDescBundles));
    }
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
    DescriptorSetBundle *referenceDescriptorSet =
        _descriptorSetBundles.empty() ? nullptr : _descriptorSetBundles[0][0].get();

    if (referenceDescriptorSet == nullptr) {
        _logger->warn(
            "No descriptor sets available, creating pipeline without descriptor reference");
    }

    _pipeline = std::make_unique<GfxPipeline>(_appContext, _logger,
                                              kPathToResourceFolder + "shaders/default",
                                              referenceDescriptorSet, _shaderCompiler, _renderPass);
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

    // Recreate buffer bundles and descriptor sets
    _createModelImages();
    _createBuffersAndBufferBundles();
    _createDescriptorSetBundles();

    // Re-allocate and re-record command buffers
    _recordDrawingCommandBuffers();
    _recordDeliveryCommandBuffers();

    _logger->info("Renderer resources have been successfully recreated.");
}

void Renderer::_updateBufferData(size_t currentFrame, size_t modelIndex, glm::mat4 modelMatrix) {
    if (modelIndex >= _renderInfoBufferBundles.size()) {
        _logger->error("Invalid model index {} in _updateBufferData", modelIndex);
        return;
    }

    auto view = _camera->getViewMatrix();

    auto swapchainExtent = _appContext->getSwapchainExtent();
    auto proj            = _camera->getProjectionMatrix();

    auto viewPos = _camera->getPosition(); // 获取摄像机位置

    S_RenderInfo renderInfo{};
    renderInfo.view    = view;
    renderInfo.proj    = proj;
    renderInfo.model   = modelMatrix;
    renderInfo.viewPos = viewPos;

    _renderInfoBufferBundles[modelIndex]->getBuffer(currentFrame)->fillData(&renderInfo);
}

void Renderer::_updateMaterialData(uint32_t currentFrame, size_t modelIndex, const glm::vec3 &color,
                                   float metallic, float roughness, float occlusion,
                                   const glm::vec3 &emissive) {
    if (modelIndex >= _materialBufferBundles.size()) {
        _logger->error("Invalid model index {} in _updateMaterialData", modelIndex);
        return;
    }
    S_MaterialInfo materialInfo{};
    materialInfo.color     = color;
    materialInfo.metallic  = metallic;
    materialInfo.roughness = roughness;
    materialInfo.occlusion = occlusion;
    materialInfo.emissive  = emissive;
    materialInfo.padding   = 0.0f; // 填充对齐

    auto &model                  = *_models[modelIndex];
    materialInfo.hasBaseColorTex = !model.baseColorTexturePaths.empty() ? 1 : 0; // 假设共享flags
    materialInfo.hasNormalTex    = !model.normalTexturePaths.empty() ? 1 : 0;
    materialInfo.hasMetalRoughnessTex = !model.metallicRoughnessTexturePaths.empty() ? 1 : 0;
    materialInfo.hasEmissiveTex       = !model.emissiveTexturePaths.empty() ? 1 : 0;

    _materialBufferBundles[modelIndex]->getBuffer(currentFrame)->fillData(&materialInfo);
}

void Renderer::updateCamera(const Transform &transform, const iCamera &camera) {
    // 更新摄像机位置
    _camera->setPosition(transform.position);

    _camera->setRotation(transform.rotation);

    // 更新摄像机投影矩阵
    _camera->setFov(camera.fov);
    _camera->setNearPlane(camera.nearPlane);
    _camera->setFarPlane(camera.farPlane);
    _camera->setAspectRatio(static_cast<float>(_appContext->getSwapchainExtent().width) /
                            static_cast<float>(_appContext->getSwapchainExtent().height));
}

void Renderer::drawFrame(size_t currentFrame, size_t imageIndex,
                         const std::vector<std::unique_ptr<Components>> &entityRenderData) {
    auto frameStartTime = std::chrono::steady_clock::now();
    DrawFrameTimings timings{};

    // Command buffer setup timing
    auto setupStart = std::chrono::steady_clock::now();
    auto &cmdBuffer = _drawingCommandBuffers[currentFrame];

    VkExtent2D currentSwapchainExtent = _appContext->getSwapchainExtent();

    VkCommandBufferBeginInfo cmdBufferBeginInfo{};
    cmdBufferBeginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufferBeginInfo.flags            = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    cmdBufferBeginInfo.pInheritanceInfo = nullptr;

    std::array<VkClearValue, 2> clear_vals = {};
    clear_vals[0].color                    = {{0.1f, 0.1f, 0.1f, 1.0f}}; // 稍微亮一点的背景色
    clear_vals[1].depthStencil             = {1.0f, 0};

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

    auto setupEnd              = std::chrono::steady_clock::now();
    timings.commandBufferSetup = _getTimeInMilliseconds(setupStart, setupEnd);

    // 新的实体驱动渲染循环
    if (_models.empty()) {
        // No models loaded, skip rendering
        vkCmdEndRenderPass(cmdBuffer);
        vkEndCommandBuffer(cmdBuffer);
        _lastFrameTimings = timings;
        return;
    }

    // Entity grouping timing
    auto groupingStart = std::chrono::steady_clock::now();
    // Group entities by modelID
    std::map<int32_t, std::vector<const Components *>> entitiesByModel;
    for (const auto &component : entityRenderData) {
        int32_t modelId = component->mesh.modelId;
        if (modelId >= 0 && static_cast<size_t>(modelId) < _models.size()) {
            entitiesByModel[modelId].push_back(component.get());
        }
    }

    auto groupingEnd       = std::chrono::steady_clock::now();
    timings.entityGrouping = _getTimeInMilliseconds(groupingStart, groupingEnd);

    // Render each model type with instanced rendering
    auto instancePrepTime = 0.0;
    auto bufferUpdateTime = 0.0;
    auto gpuCommandTime   = 0.0;

    for (const auto &[modelId, entities] : entitiesByModel) {
        size_t modelIndex          = static_cast<size_t>(modelId);
        const size_t instanceCount = entities.size();

        if (instanceCount == 0) continue;

        // Instance data preparation timing
        auto instancePrepStart = std::chrono::steady_clock::now();
        // Prepare instance data (model matrices)
        std::vector<glm::mat4> instanceMatrices;
        instanceMatrices.reserve(instanceCount);

        // For material data, we'll use the first entity's material for all instances
        // (This assumes all instances of the same model use the same material)
        const auto &firstEntity = entities[0];

        for (const auto &entity : entities) {
            glm::mat4 finalMatrix = glm::translate(glm::mat4(1.0f), entity->transform.position);

            // 旋转
            finalMatrix =
                glm::rotate(finalMatrix, entity->transform.rotation.x, glm::vec3(1, 0, 0));
            finalMatrix =
                glm::rotate(finalMatrix, entity->transform.rotation.y, glm::vec3(0, 1, 0));
            finalMatrix =
                glm::rotate(finalMatrix, entity->transform.rotation.z, glm::vec3(0, 0, 1));

            // 缩放
            finalMatrix = glm::scale(finalMatrix, entity->transform.scale);

            instanceMatrices.push_back(finalMatrix);
        }

        auto instancePrepEnd = std::chrono::steady_clock::now();
        instancePrepTime += _getTimeInMilliseconds(instancePrepStart, instancePrepEnd);

        // Buffer update timing
        auto bufferUpdateStart = std::chrono::steady_clock::now();
        // Upload instance matrices to instance buffer
        auto instanceBuffer = _instanceBufferBundles[modelIndex]->getBuffer(currentFrame);
        instanceBuffer->fillData(instanceMatrices.data());

        // Update camera and material data (shared across all instances)
        _updateBufferData(currentFrame, modelIndex,
                          glm::mat4(1.0f)); // Identity matrix since we use instance matrices
        _updateMaterialData(currentFrame, modelIndex, firstEntity->material.color,
                            firstEntity->material.metallic, firstEntity->material.roughness,
                            firstEntity->material.occlusion, firstEntity->material.emissive);

        auto bufferUpdateEnd = std::chrono::steady_clock::now();
        bufferUpdateTime += _getTimeInMilliseconds(bufferUpdateStart, bufferUpdateEnd);

        auto gpuCommandStart = std::chrono::steady_clock::now();
        // Bind descriptor sets - but since per mesh, moved inside mesh loop

        // Bind instance buffer (binding 1 for instance data)
        VkBuffer instanceBuffers[] = {
            _instanceBufferBundles[modelIndex]->getBuffer(currentFrame)->getVkBuffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmdBuffer, 1, 1, instanceBuffers, offsets);

        // Loop over meshes in the model
        auto &model = *_models[modelIndex];
        for (size_t meshIdx = 0; meshIdx < model.idxCnts.size(); ++meshIdx) {
            // Bind per mesh descriptor set
            vkCmdBindDescriptorSets(
                cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline->getPipelineLayout(), 0, 1,
                &_descriptorSetBundles[modelIndex][meshIdx]->getDescriptorSet(currentFrame), 0,
                nullptr);

            // Bind vertex buffer (binding 0 for vertex data)
            VkBuffer vertexBuffers[] = {model.vertexBuffers[meshIdx]->getVkBuffer()};
            vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets);

            vkCmdBindIndexBuffer(cmdBuffer, model.indexBuffers[meshIdx]->getVkBuffer(), 0,
                                 VK_INDEX_TYPE_UINT32);

            vkCmdDrawIndexed(cmdBuffer, model.idxCnts[meshIdx],
                             static_cast<uint32_t>(instanceCount), 0, 0, 0);
        }

        auto gpuCommandEnd = std::chrono::steady_clock::now();
        gpuCommandTime += _getTimeInMilliseconds(gpuCommandStart, gpuCommandEnd);
    }

    // Store accumulated timing data
    timings.instanceDataPrep    = instancePrepTime;
    timings.bufferUpdates       = bufferUpdateTime;
    timings.gpuCommandRecording = gpuCommandTime;

    // Command buffer finish timing
    auto finishStart = std::chrono::steady_clock::now();
    vkCmdEndRenderPass(cmdBuffer);
    vkEndCommandBuffer(cmdBuffer);
    auto finishEnd = std::chrono::steady_clock::now();

    timings.commandBufferFinish = _getTimeInMilliseconds(finishStart, finishEnd);
    timings.totalDrawFrame      = _getTimeInMilliseconds(frameStartTime, finishEnd);

    // Store timing results
    _lastFrameTimings = timings;
}

double Renderer::_getTimeInMilliseconds(std::chrono::steady_clock::time_point start,
                                        std::chrono::steady_clock::time_point end) const {
    return std::chrono::duration<double, std::milli>(end - start).count();
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

    // 记录命令
    for (size_t i = 0; i < _deliveryCommandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        vkBeginCommandBuffer(_deliveryCommandBuffers[i], &beginInfo);

        // 转换图像布局
        VkImageMemoryBarrier barrier{};
        barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout                       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.newLayout                       = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.image                           = _appContext->getSwapchainImages()[i];
        barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = 1;
        barrier.srcAccessMask                   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask                   = 0;

        vkCmdPipelineBarrier(_deliveryCommandBuffers[i], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0,
                             nullptr, 1, &barrier);

        vkEndCommandBuffer(_deliveryCommandBuffers[i]);
    }
}

void Renderer::_createDefaultTextures() {
    auto samplerSettings = Sampler::Settings{
        Sampler::AddressMode::kClampToEdge, // U
        Sampler::AddressMode::kClampToEdge, // V
        Sampler::AddressMode::kClampToEdge  // W
    };
    _defaultSampler = std::make_unique<Sampler>(_appContext, samplerSettings);

    // 默认baseColor: 白色
    _defaultBaseColorTexture = std::make_unique<Image>(
        _appContext, _logger, ImageDimensions{1, 1, 1}, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        _defaultSampler->getVkSampler());
    std::array<uint8_t, 4> whitePixel = {255, 255, 255, 255};
    _uploadTextureData(_defaultBaseColorTexture.get(), whitePixel.data());

    // 默认normal: (0.5, 0.5, 1.0)
    _defaultNormalTexture = std::make_unique<Image>(
        _appContext, _logger, ImageDimensions{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        _defaultSampler->getVkSampler());
    std::array<uint8_t, 4> normalPixel = {128, 128, 255, 255};
    _uploadTextureData(_defaultNormalTexture.get(), normalPixel.data());

    // 默认metalRoughness: (0, 1, 0, 1) - 非金属、粗糙
    _defaultMetalRoughnessTexture = std::make_unique<Image>(
        _appContext, _logger, ImageDimensions{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        _defaultSampler->getVkSampler());
    std::array<uint8_t, 4> mrPixel = {0, 255, 0, 255};
    _uploadTextureData(_defaultMetalRoughnessTexture.get(), mrPixel.data());

    // 默认emissive: 黑色
    _defaultEmissiveTexture = std::make_unique<Image>(
        _appContext, _logger, ImageDimensions{1, 1, 1}, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        _defaultSampler->getVkSampler());
    std::array<uint8_t, 4> blackPixel = {0, 0, 0, 255};
    _uploadTextureData(_defaultEmissiveTexture.get(), blackPixel.data());
}

void Renderer::_uploadTextureData(Image *image, const void *pixelData) {
    if (image->getVkImage() == VK_NULL_HANDLE) return;
    // 创建staging buffer
    Buffer stagingBuffer(_appContext, 4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         MemoryStyle::kHostVisible);
    stagingBuffer.fillData(pixelData);

    // 用命令缓冲区复制到image（假设用单次命令缓冲区）
    VkCommandBuffer cmd = _appContext->beginSingleTimeCommands();

    VkBufferImageCopy copyRegion{};
    copyRegion.bufferOffset                    = 0;
    copyRegion.bufferRowLength                 = 0;
    copyRegion.bufferImageHeight               = 0;
    copyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.mipLevel       = 0;
    copyRegion.imageSubresource.baseArrayLayer = 0;
    copyRegion.imageSubresource.layerCount     = 1;
    copyRegion.imageOffset                     = {0, 0, 0};
    copyRegion.imageExtent                     = {1, 1, 1};

    VkImageMemoryBarrier barrier{};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = image->getVkImage();
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;
    barrier.srcAccessMask                   = 0;
    barrier.dstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                         0, nullptr, 0, nullptr, 1, &barrier);

    vkCmdCopyBufferToImage(cmd, stagingBuffer.getVkBuffer(), image->getVkImage(),
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);

    _appContext->endSingleTimeCommands(cmd);
}