#pragma once
#include <memory>
#include <vector>

#include "utils/vulkan-wrapper/memory/Image.hpp"
#include "utils/vulkan-wrapper/sampler/Sampler.hpp"

class VulkanApplicationContext;
class Logger;
class ShaderCompiler;
class Window;
class ConfigContainer;
class Camera;
class Model;
class BufferBundle;
class DescriptorSetBundle;
class GfxPipeline;

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

    void drawFrame(size_t currentFrame, size_t imageIndex, glm::mat4 modelMatrix);
    void processInput(double deltaTime);
    void onSwapchainResize();
    [[nodiscard]] inline VkCommandBuffer getDrawingCommandBuffer(size_t currentFrame) {
        return _drawingCommandBuffers[currentFrame];
    }

    VkCommandBuffer getTracingCommandBuffer(size_t frame) const { return _tracingCommandBuffers[frame]; }
    VkCommandBuffer getDeliveryCommandBuffer(size_t index) const { return _deliveryCommandBuffers[index]; }

private:
    struct Images {
        std::unique_ptr<Sampler> sharedSampler;
        std::vector<std::unique_ptr<Image>> baseColors;
        std::vector<std::unique_ptr<Image>> emissiveTextures;      // 发光贴图
        std::vector<std::unique_ptr<Image>> metallicRoughnessTextures; // 金属粗糙度贴图
        std::vector<std::unique_ptr<Image>> normalTextures;       // 法线贴图
    } _images;

    VulkanApplicationContext *_appContext;
    Logger *_logger;
    size_t _framesInFlight;
    ShaderCompiler *_shaderCompiler;
    Window *_window;
    ConfigContainer *_configContainer;

    std::unique_ptr<Camera> _camera;
    std::unique_ptr<Image> _renderTargetImage;
    std::unique_ptr<Model> _model;
    std::unique_ptr<BufferBundle> _renderInfoBufferBundle;
    std::unique_ptr<DescriptorSetBundle> _descriptorSetBundle;
    std::unique_ptr<GfxPipeline> _pipeline;
    std::unique_ptr<Image> _depthStencilImage;
    std::unique_ptr<Image> _colorResourcesImage;
    std::vector<VkCommandBuffer> _deliveryCommandBuffers{};
    std::vector<VkCommandBuffer> _drawingCommandBuffers{};
    std::vector<VkFramebuffer> _frameBuffers{};

    std::unique_ptr<Model> _model = nullptr;
    struct {
        std::unique_ptr<Sampler> sharedSampler;
        std::unique_ptr<Image> baseColor;
        std::unique_ptr<Image> normalMap;
        std::unique_ptr<Image> metalRoughness;
    } _images;

    VkRenderPass _renderPass;
    std::vector<VkFramebuffer> _frameBuffers;
    std::vector<VkCommandBuffer> _tracingCommandBuffers;
    std::vector<VkCommandBuffer> _deliveryCommandBuffers;

    void _createBuffersAndBufferBundles();
    void _createDescriptorSetBundle();
    void _updateBufferData(size_t currentFrame, glm::mat4 model_matrix);

    // descriptor set
    std::unique_ptr<DescriptorSetBundle> _descriptorSetBundle;

    void _recordDeliveryCommandBuffers();
    void _recordDrawingCommandBuffers();
    void _createRenderPass();
    void _createGraphicsPipeline();
    void _createDepthStencil();
    void _createColorResources();
    void _createFrameBuffers();
    void _updateUboData(size_t currentFrame);
    void _recordTracingCommandBuffers();
    void _recordDeliveryCommandBuffers();
};