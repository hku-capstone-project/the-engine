#pragma once

#include "Pipeline.hpp"
#include "glm/glm.hpp"
#include "vma/vk_mem_alloc.h"

class ShaderCompiler;

struct MVP {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

// GFX shaders should be placed in a folder and name as vert.glsl & frag.glsl
class GfxPipeline : public Pipeline {
  public:
    GfxPipeline(VulkanApplicationContext *appContext, Logger *logger, glm::vec3 workGroupSize,
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

    void updateMVP(const MVP& mvp);

  private:
    VkShaderModule _vertShaderModule = VK_NULL_HANDLE;
    VkShaderModule _fragShaderModule = VK_NULL_HANDLE;
    glm::vec3 _workGroupSize;
    ShaderCompiler *_shaderCompiler;
    VkPipelineCache _pipelineCache = VK_NULL_HANDLE;
    VkRenderPass _renderPass       = VK_NULL_HANDLE;

    VkBuffer _mvpBuffer = VK_NULL_HANDLE;
    VmaAllocation _mvpBufferAllocation = VK_NULL_HANDLE;
    void* _mvpBufferMapped = nullptr;

    void _cleanupShaderModules() override;
};
