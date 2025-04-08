#pragma once

#include "Pipeline.hpp"

struct WorkGroupSize {
  uint32_t x;
  uint32_t y;
  uint32_t z;
};

class ShaderCompiler;

class ComputePipeline : public Pipeline {
public:
  ComputePipeline(VulkanApplicationContext *appContext, Logger *logger, WorkGroupSize workGroupSize,
                  DescriptorSetBundle *descriptorSetBundle, ShaderCompiler *shaderCompiler);

  ~ComputePipeline() override;

  // disable copy and move
  ComputePipeline(const ComputePipeline &)            = delete;
  ComputePipeline &operator=(const ComputePipeline &) = delete;
  ComputePipeline(ComputePipeline &&)                 = delete;
  ComputePipeline &operator=(ComputePipeline &&)      = delete;

  void build() override;
  void compileAndCacheShaderModule(std::string &path) override;

  void recordCommand(VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t threadCountX,
                     uint32_t threadCountY, uint32_t threadCountZ);

  void recordIndirectCommand(VkCommandBuffer commandBuffer, uint32_t currentFrame,
                             VkBuffer indirectBuffer);

private:
  VkShaderModule _cachedShaderModule = VK_NULL_HANDLE;
  WorkGroupSize _workGroupSize;
  ShaderCompiler *_shaderCompiler;

  void _cleanupShaderModules() override;
};
