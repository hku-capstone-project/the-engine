#pragma once

#include "Pipeline.hpp"
#include "utils/incl/GlmIncl.hpp" // IWYU pragma: export
#include "vma/vk_mem_alloc.h"

class ShaderCompiler;

// GFX shaders should be placed in a folder and name as vert.glsl & frag.glsl
class GfxPipeline : public Pipeline {
  public:
    GfxPipeline(VulkanApplicationContext *appContext, Logger *logger,
                std::string fullPathToShaderSourceCode, DescriptorSetBundle *descriptorSetBundle,
                Image *baseColor, ShaderCompiler *shaderCompiler, VkRenderPass renderPass);

    ~GfxPipeline() override;

    GfxPipeline(const GfxPipeline &)            = delete;
    GfxPipeline &operator=(const GfxPipeline &) = delete;
    GfxPipeline(GfxPipeline &&)                 = delete;
    GfxPipeline &operator=(GfxPipeline &&)      = delete;

    void build() override;
    void compileAndCacheShaderModule() override;

  private:
    ShaderCompiler *_shaderCompiler;

    VkShaderModule _vertShaderModule = VK_NULL_HANDLE;
    VkShaderModule _fragShaderModule = VK_NULL_HANDLE;

    VkPipelineCache _pipelineCache = VK_NULL_HANDLE;

    VkRenderPass _renderPass;

    void _cleanupShaderModules() override;
};
