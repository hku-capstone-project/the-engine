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
             std::string fullPathToShaderSourceCode, DescriptorSetBundle *descriptorSetBundle,
             VkShaderStageFlags shaderStageFlags);
    virtual ~Pipeline();

    // disable copy and move
    Pipeline(const Pipeline &)            = delete;
    Pipeline &operator=(const Pipeline &) = delete;
    Pipeline(Pipeline &&)                 = delete;
    Pipeline &operator=(Pipeline &&)      = delete;

    virtual void build() = 0;

    // build the shader module and cache it
    // returns of the shader has been compiled and cached correctly
    virtual void compileAndCacheShaderModule() = 0;

    void updateDescriptorSetBundle(DescriptorSetBundle *descriptorSetBundle);

    [[nodiscard]] std::string getFullPathToShaderSourceCode() const {
        return _fullPathToShaderSourceCode;
    }

    [[nodiscard]] const inline VkPipeline &getPipeline() const { return _pipeline; }
    [[nodiscard]] const inline VkPipelineLayout &getPipelineLayout() const {
        return _pipelineLayout;
    }

    void recordBind(VkCommandBuffer commandBuffer, size_t currentFrame);

  protected:
    VulkanApplicationContext *_appContext;
    Logger *_logger;

    VkShaderStageFlags _shaderStageFlags;

    DescriptorSetBundle *_descriptorSetBundle;
    std::string _fullPathToShaderSourceCode;

    VkPipeline _pipeline             = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;

    void _cleanupPipelineAndLayout();
    virtual void _cleanupShaderModules() = 0;
    void _doCleanupShaderModules();

    VkShaderModule _createShaderModule(const std::vector<uint32_t> &code);
};
