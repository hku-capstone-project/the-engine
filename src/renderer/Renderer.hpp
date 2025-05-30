#pragma once
#define VK_NO_PROTOTYPES

#include "utils/vulkan-wrapper/pipeline/GfxPipeline.hpp"
#include "vma/vk_mem_alloc.h"
#include "volk.h"

#include <memory>
#include <vector>

class VulkanApplicationContext;
class Logger;
class ShaderCompiler;
class Window;
class ConfigContainer;
class Model;
class Image;
class ImageForwardingPair;
class BufferBundle;
class DescriptorSetBundle;
class Camera;

class Renderer {
  public:
    Renderer(VulkanApplicationContext *appContext, Logger *logger, size_t framesInFlight,
             ShaderCompiler *shaderCompiler, Window *window, ConfigContainer *configContainer);
    ~Renderer();

    // disable move and copy
    Renderer(const Renderer &)            = delete;
    Renderer &operator=(const Renderer &) = delete;
    Renderer(Renderer &&)                 = delete;
    Renderer &operator=(Renderer &&)      = delete;

    void drawFrame(size_t currentFrame, size_t imageIndex);
    void processInput(double deltaTime);

    void onSwapchainResize();
    [[nodiscard]] inline VkCommandBuffer getTracingCommandBuffer(size_t currentFrame) {
        return _tracingCommandBuffers[currentFrame];
    }

    [[nodiscard]] inline VkCommandBuffer getDeliveryCommandBuffer(size_t imageIndex) {
        return _deliveryCommandBuffers[imageIndex];
    }

  private:
    VulkanApplicationContext *_appContext;
    Logger *_logger;
    ShaderCompiler *_shaderCompiler;
    Window *_window;
    std::unique_ptr<Camera> _camera;

    ConfigContainer *_configContainer;

    std::vector<VkCommandBuffer> _deliveryCommandBuffers{};
    std::vector<VkCommandBuffer> _tracingCommandBuffers{};
    std::vector<VkFramebuffer> _frameBuffers{};

    std::unique_ptr<Model> _model = nullptr;
    struct {
        std::unique_ptr<Image> baseColor;
        std::unique_ptr<Image> normalMap;
        std::unique_ptr<Image> metalRoughness;
    } _images;

    VkRenderPass _renderPass;

    std::unique_ptr<Image> _renderTargetImage   = nullptr;
    std::unique_ptr<Image> _depthStencilImage   = nullptr;
    std::unique_ptr<Image> _colorResourcesImage = nullptr;

    size_t _framesInFlight = 0;

    // pipeline
    std::unique_ptr<GfxPipeline> _pipeline = nullptr;
    void _createGraphicsPipeline();

    // buffers
    std::unique_ptr<BufferBundle> _renderInfoBufferBundle;
    void _createBuffersAndBufferBundles();

    // descriptor set
    std::unique_ptr<DescriptorSetBundle> _descriptorSetBundle;

    void _recordDeliveryCommandBuffers();
    void _recordTracingCommandBuffers();
    void _createRenderPass();
    void _createFrameBuffers();
    void _createDepthStencil();
    void _createColorResources();

    void _createDescriptorSetBundle();
};
