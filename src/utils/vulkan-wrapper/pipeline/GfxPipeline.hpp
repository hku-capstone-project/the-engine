#pragma once

#include "Pipeline.hpp"
#include "utils/incl/GlmIncl.hpp" // IWYU pragma: export
#include "vma/vk_mem_alloc.h"

#include <memory>

class Model;
class ShaderCompiler;
class Buffer;

// GFX shaders should be placed in a folder and name as vert.glsl & frag.glsl
class GfxPipeline : public Pipeline {
  public:
    GfxPipeline(VulkanApplicationContext *appContext, Logger *logger,
                std::string fullPathToShaderSourceCode, DescriptorSetBundle *descriptorSetBundle,
                glm::vec3 workGroupSize, Image *baseColor, ShaderCompiler *shaderCompiler,
                VkRenderPass renderPass);

    ~GfxPipeline() override;

    GfxPipeline(const GfxPipeline &)            = delete;
    GfxPipeline &operator=(const GfxPipeline &) = delete;
    GfxPipeline(GfxPipeline &&)                 = delete;
    GfxPipeline &operator=(GfxPipeline &&)      = delete;

    void build() override;
    void compileAndCacheShaderModule() override;

    void recordCommand(VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t threadCountX,
                       uint32_t threadCountY, uint32_t threadCountZ);

    void recordIndirectCommand(VkCommandBuffer commandBuffer, uint32_t currentFrame,
                               VkBuffer indirectBuffer);

  private:
    VkShaderModule _vertShaderModule = VK_NULL_HANDLE;
    VkShaderModule _fragShaderModule = VK_NULL_HANDLE;
    glm::vec3 _workGroupSize;
    ShaderCompiler *_shaderCompiler;
    VkPipelineCache _pipelineCache = VK_NULL_HANDLE;

    struct {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    } _mvp;

    std::unique_ptr<Buffer> _mvpBuffer = VK_NULL_HANDLE;
    VmaAllocation _mvpBufferAllocation = VK_NULL_HANDLE;
    VkRenderPass _renderPass;

    void _cleanupShaderModules() override;
};
