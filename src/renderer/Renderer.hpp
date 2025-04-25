#pragma once

#include "volk.h"
#include <memory>
#include <vector>

class VulkanApplicationContext;
class Logger;
class ShaderCompiler;
class Window;
class ConfigContainer;
class Image;
class ImageForwardingPair;

class Renderer {
public:
  Renderer(VulkanApplicationContext *appContext, Logger *logger, ShaderCompiler *shaderCompiler,
           Window *window, ConfigContainer *configContainer);
  ~Renderer();

  // disable move and copy
  Renderer(const Renderer &)            = delete;
  Renderer &operator=(const Renderer &) = delete;
  Renderer(Renderer &&)                 = delete;
  Renderer &operator=(Renderer &&)      = delete;

  void drawFrame(size_t currentFrame);
  void processInput(double deltaTime);

  void onSwapchainResize();
  // VkCommandBuffer getTracingCommandBuffer(size_t currentFrame) {
  //   return _tracingCommandBuffers[currentFrame];
  // }

  VkCommandBuffer getDeliveryCommandBuffer(size_t imageIndex) {
    return _deliveryCommandBuffers[imageIndex];
  }

private:
  VulkanApplicationContext *_appContext;
  Logger *_logger;
  ShaderCompiler *_shaderCompiler;
  Window *_window;
  ConfigContainer *_configContainer;

  std::vector<VkCommandBuffer> _deliveryCommandBuffers{};

  std::unique_ptr<Image> _renderTargetImage = nullptr;
  std::vector<std::unique_ptr<ImageForwardingPair>> _targetForwardingPairs;

  void _recordDeliveryCommandBuffers();
};
