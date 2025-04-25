#include "Renderer.hpp"
#include "app-context/VulkanApplicationContext.hpp"
#include "config-container/ConfigContainer.hpp"
#include "utils/logger/Logger.hpp"
#include "utils/shader-compiler/ShaderCompiler.hpp"
#include "utils/vulkan-wrapper/memory/Image.hpp"
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

  _targetForwardingPairs.clear();
  for (int i = 0; i < _appContext->getSwapchainImagesCount(); i++) {
    _targetForwardingPairs.emplace_back(std::make_unique<ImageForwardingPair>(
        _renderTargetImage->getVkImage(), _appContext->getSwapchainImages()[i],
        _renderTargetImage->getDimensions(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
  }

  _recordDeliveryCommandBuffers();
}

Renderer::~Renderer() = default;

void Renderer::onSwapchainResize() {
  // TODO:
}

void Renderer::drawFrame(size_t currentFrame) {}

void Renderer::processInput(double deltaTime) {
  // _camera->processInput(deltaTime);
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

  VkMemoryBarrier deliveryMemoryBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
  deliveryMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  deliveryMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

  for (size_t imageIndex = 0; imageIndex < _deliveryCommandBuffers.size(); imageIndex++) {
    auto &cmdBuffer = _deliveryCommandBuffers[imageIndex];

    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    vkBeginCommandBuffer(cmdBuffer, &beginInfo);

    // // make all host writes to the ubo visible to the shaders
    // vkCmdPipelineBarrier(cmdBuffer,
    //                      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, // source stage
    //                      VK_PIPELINE_STAGE_TRANSFER_BIT,       // destination stage
    //                      0,                                    // dependency flags
    //                      1,                                    // memory barrier count
    //                      &deliveryMemoryBarrier,               // memory barriers
    //                      0,                                    // buffer memory barrier count
    //                      nullptr,                              // buffer memory barriers
    //                      0,                                    // image memory barrier count
    //                      nullptr                               // image memory barriers
    // );

    _targetForwardingPairs[imageIndex]->forwardCopy(cmdBuffer);
    vkEndCommandBuffer(cmdBuffer);
  }
}
