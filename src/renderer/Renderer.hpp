#pragma once
#define VK_NO_PROTOTYPES

#include "dotnet/Components.hpp"
#include "utils/vulkan-wrapper/pipeline/GfxPipeline.hpp"
#include "vma/vk_mem_alloc.h"
#include "volk.h"


#include "utils/incl/GlmIncl.hpp" // IWYU pragma: keep
#include <chrono>
#include <memory>
#include <utility>
#include <vector>

class RuntimeApplication;

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
class Sampler;

// Detailed timing measurements
struct DrawFrameTimings {
    double commandBufferSetup  = 0.0;
    double entityGrouping      = 0.0;
    double instanceDataPrep    = 0.0;
    double bufferUpdates       = 0.0;
    double gpuCommandRecording = 0.0;
    double commandBufferFinish = 0.0;
    double totalDrawFrame      = 0.0;
};

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

    // update camera position and projection matrix
    void updateCamera(const Transform &transform, const iCamera &camera);

    void drawFrame(size_t currentFrame, size_t imageIndex,
                   const std::vector<std::unique_ptr<Components>> &entityRenderData);
    void processInput(double deltaTime);

    void onSwapchainResize();
    [[nodiscard]] inline VkCommandBuffer getDrawingCommandBuffer(size_t currentFrame) {
        return _drawingCommandBuffers[currentFrame];
    }

    [[nodiscard]] inline VkCommandBuffer getDeliveryCommandBuffer(size_t imageIndex) {
        return _deliveryCommandBuffers[imageIndex];
    }

    // Get last frame's detailed timing results
    [[nodiscard]] inline const DrawFrameTimings &getLastFrameTimings() const {
        return _lastFrameTimings;
    }

  private:
    VulkanApplicationContext *_appContext;
    Logger *_logger;
    ShaderCompiler *_shaderCompiler;
    Window *_window;
    std::unique_ptr<Camera> _camera;

    ConfigContainer *_configContainer;

    std::vector<VkCommandBuffer> _deliveryCommandBuffers{};
    std::vector<VkCommandBuffer> _drawingCommandBuffers{};
    std::vector<VkFramebuffer> _frameBuffers{};

    std::vector<std::unique_ptr<Model>> _models{};

    struct ModelImages {
        std::unique_ptr<Sampler> sharedSampler;
        std::unique_ptr<Image> baseColor;
        std::unique_ptr<Image> normalMap;
        std::unique_ptr<Image> metalRoughness;
        std::unique_ptr<Image> emissive;
    };
    std::vector<std::vector<ModelImages>> _modelImages{};  // per model per mesh

    std::unique_ptr<Sampler> _defaultSampler = nullptr;
    std::unique_ptr<Image> _defaultBaseColorTexture = nullptr;
    std::unique_ptr<Image> _defaultNormalTexture = nullptr;
    std::unique_ptr<Image> _defaultMetalRoughnessTexture = nullptr;
    std::unique_ptr<Image> _defaultEmissiveTexture = nullptr;

    VkRenderPass _renderPass;

    std::unique_ptr<Image> _renderTargetImage   = nullptr;
    std::unique_ptr<Image> _depthStencilImage   = nullptr;
    std::unique_ptr<Image> _colorResourcesImage = nullptr;

    size_t _framesInFlight = 0;

    // pipeline
    std::unique_ptr<GfxPipeline> _pipeline = nullptr;
    void _createGraphicsPipeline();

    // buffers
    std::vector<std::unique_ptr<BufferBundle>> _renderInfoBufferBundles;
    std::vector<std::vector<std::unique_ptr<DescriptorSetBundle>>> _descriptorSetBundles;
    std::vector<std::vector<std::unique_ptr<BufferBundle>>> _materialBufferBundles;
    std::vector<std::unique_ptr<BufferBundle>> _instanceBufferBundles;

    mutable DrawFrameTimings _lastFrameTimings;
    double _getTimeInMilliseconds(std::chrono::steady_clock::time_point start,
                                  std::chrono::steady_clock::time_point end) const;

    void _recordDeliveryCommandBuffers();
    void _recordDrawingCommandBuffers();
    void _createRenderPass();
    void _createFrameBuffers();
    void _createDepthStencil();
    void _createColorResources();

    void _createDescriptorSetBundles();
    void _createModelImages();
    void _createBuffersAndBufferBundles();
    void _updateBufferData(size_t currentFrame, size_t modelIndex, glm::mat4 model_matrix);
    void _updateMaterialData(uint32_t currentFrame, size_t modelIndex, size_t meshIndex,
                                       const glm::vec3 &color, float metallic, float roughness,
                                       float occlusion, const glm::vec3 &emissive);
    void _createDefaultTextures();
    void _uploadTextureData(Image *image, const void *pixelData);  // 新增helper上传像素
};