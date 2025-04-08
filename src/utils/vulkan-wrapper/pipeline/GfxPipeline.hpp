#pragma once

#include "Pipeline.hpp"

struct WorkGroupSize {
  uint32_t x;
  uint32_t y;
  uint32_t z;
};

class ShaderCompiler;

// GFX shaders should be placed in a folder and name as vert.glsl & frag.glsl
class GfxPipeline: public Pipeline {
public:
  GfxPipeline(VulkanApplicationContext *appContext, Logger *logger, WorkGroupSize workGroupSize,
                  DescriptorSetBundle *descriptorSetBundle, ShaderCompiler *shaderCompiler);

  ~GfxPipeline() override;

  GfxPipeline(const GfxPipeline &)            = delete;
  GfxPipeline &operator=(const GfxPipeline &) = delete;
  GfxPipeline(GfxPipeline &&)                 = delete;
  GfxPipeline &operator=(GfxPipeline &&)      = delete;

  void build() override;
  void compileAndCacheShaderModule(std::string &path) override;

  void recordCommand(VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t threadCountX,
                     uint32_t threadCountY, uint32_t threadCountZ);

  void recordIndirectCommand(VkCommandBuffer commandBuffer, uint32_t currentFrame,
                             VkBuffer indirectBuffer);

private:
  VkShaderModule _vertShaderModule = VK_NULL_HANDLE;
  VkShaderModule _fragShaderModule = VK_NULL_HANDLE;
  WorkGroupSize _workGroupSize;
  ShaderCompiler *_shaderCompiler;
  VkPipelineCache _pipelineCache = VK_NULL_HANDLE;
  VkRenderPass _renderPass = VK_NULL_HANDLE;

  void _cleanupShaderModules() override;
};