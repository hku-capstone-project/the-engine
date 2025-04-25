#pragma once

#include "volk.h"

#include <cstdint>
#include <string>
#include <vector>

class Logger;
class Image;
class BufferBundle;
class VulkanApplicationContext;
class DescriptorSetBundle;

class Pipeline {
public:
  Pipeline(VulkanApplicationContext *appContext, Logger *logger,
           DescriptorSetBundle *descriptorSetBundle, VkShaderStageFlags shaderStageFlags);
  virtual ~Pipeline();

  // disable copy and move
  Pipeline(const Pipeline &)            = delete;
  Pipeline &operator=(const Pipeline &) = delete;
  Pipeline(Pipeline &&)                 = delete;
  Pipeline &operator=(Pipeline &&)      = delete;

  virtual void build() = 0;
  void init(std::string &path);

  // build the shader module and cache it
  // returns of the shader has been compiled and cached correctly
  virtual void compileAndCacheShaderModule(std::string &path) = 0;

  void updateDescriptorSetBundle(DescriptorSetBundle *descriptorSetBundle);

protected:
  VulkanApplicationContext *_appContext;
  Logger *_logger;

  DescriptorSetBundle *_descriptorSetBundle;

  std::vector<BufferBundle *> _uniformBufferBundles; // buffer bundles for uniform data
  std::vector<BufferBundle *> _storageBufferBundles; // buffer bundles for storage data
  std::vector<Image *> _storageImages;               // images for storage data

  VkShaderStageFlags _shaderStageFlags;

  VkPipeline _pipeline             = VK_NULL_HANDLE;
  VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;

  void _cleanupPipelineAndLayout();
  virtual void _cleanupShaderModules() = 0;
  void _doCleanupShaderModules();

  VkShaderModule _createShaderModule(const std::vector<uint32_t> &code);
  void _bind(VkCommandBuffer commandBuffer, size_t currentFrame);
};
