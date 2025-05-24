#pragma once

#include "volk.h"
#include <memory>
#include <vector>

class GfxPipeline;
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

  void drawFrame(size_t currentFrame);
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
  size_t _framesInFlight;
  ShaderCompiler *_shaderCompiler;
  Window *_window;
  ConfigContainer *_configContainer;
  std::unique_ptr<GfxPipeline> _pipeline = nullptr;

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

  std::unique_ptr<Image> _renderTargetImage = nullptr;

  // BUFFERS

  std::unique_ptr<BufferBundle> _guiInputBufferBundle = nullptr;

  void _createBuffersAndBufferBundles();

  // DESCRIPTOR SETS

  std::unique_ptr<DescriptorSetBundle> _descriptorSetBundle = nullptr;

  void _createDescriptorSetBundle();

  struct {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView imageView;
  } depthStencil;

  void _recordDeliveryCommandBuffers();
  void _recordTracingCommandBuffers();
  void _createRenderPass();
  void _createFrameBuffers();
  void _createDepthStencil();
};
