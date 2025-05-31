#include "Pipeline.hpp"
#include "../descriptor-set/DescriptorSetBundle.hpp"
#include "app-context/VulkanApplicationContext.hpp"
#include "utils/logger/Logger.hpp"

#include <map>
#include <vector>

static const std::map<VkShaderStageFlags, VkPipelineBindPoint> kShaderStageFlagsToBindPoint{
    {VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, VK_PIPELINE_BIND_POINT_GRAPHICS},
    {VK_SHADER_STAGE_COMPUTE_BIT, VK_PIPELINE_BIND_POINT_COMPUTE}};

Pipeline::Pipeline(VulkanApplicationContext *appContext, Logger *logger,
                   std::string fullPathToShaderSourceCode, DescriptorSetBundle *descriptorSetBundle,
                   VkShaderStageFlags shaderStageFlags)
    : _appContext(appContext), _logger(logger),
      _fullPathToShaderSourceCode(fullPathToShaderSourceCode),
      _descriptorSetBundle(descriptorSetBundle), _shaderStageFlags(shaderStageFlags) {}

Pipeline::~Pipeline() {
    _doCleanupShaderModules();
    _cleanupPipelineAndLayout();
}

void Pipeline::_doCleanupShaderModules() { _cleanupShaderModules(); }

void Pipeline::_cleanupPipelineAndLayout() {
    if (_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(_appContext->getDevice(), _pipelineLayout, nullptr);
        _pipelineLayout = VK_NULL_HANDLE;
    }
    if (_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(_appContext->getDevice(), _pipeline, nullptr);
        _pipeline = VK_NULL_HANDLE;
    }
}

void Pipeline::updateDescriptorSetBundle(DescriptorSetBundle *descriptorSetBundle) {
    _descriptorSetBundle = descriptorSetBundle;
    build();
}

VkShaderModule Pipeline::_createShaderModule(const std::vector<uint32_t> &code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pCode    = code.data();
    createInfo.codeSize = sizeof(uint32_t) * code.size();

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    vkCreateShaderModule(_appContext->getDevice(), &createInfo, nullptr, &shaderModule);
    return shaderModule;
}

void Pipeline::recordBind(VkCommandBuffer commandBuffer, size_t currentFrame) {
    vkCmdBindDescriptorSets(commandBuffer, kShaderStageFlagsToBindPoint.at(_shaderStageFlags),
                            _pipelineLayout, 0, 1,
                            &_descriptorSetBundle->getDescriptorSet(currentFrame), 0, nullptr);
    vkCmdBindPipeline(commandBuffer, kShaderStageFlagsToBindPoint.at(_shaderStageFlags), _pipeline);
}
