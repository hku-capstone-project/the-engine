#include "ComputePipeline.hpp"

#include "app-context/VulkanApplicationContext.hpp"

#include "../descriptor-set/DescriptorSetBundle.hpp"
#include "utils/io/FileReader.hpp"
#include "utils/logger/Logger.hpp"
#include "utils/shader-compiler/ShaderCompiler.hpp"

ComputePipeline::ComputePipeline(VulkanApplicationContext *appContext, Logger *logger,
                                 WorkGroupSize workGroupSize,
                                 DescriptorSetBundle *descriptorSetBundle,
                                 ShaderCompiler *shaderCompiler)
    : Pipeline(appContext, logger, descriptorSetBundle, VK_SHADER_STAGE_COMPUTE_BIT),
      _workGroupSize(workGroupSize), _shaderCompiler(shaderCompiler) {}

ComputePipeline::~ComputePipeline() = default;

void ComputePipeline::compileAndCacheShaderModule(std::string &path) {
    auto const sourceCode = FileReader::readShaderSourceCode(path, _logger);
    auto const compiledCode =
        _shaderCompiler->compileShaderFromFile(ShaderStage::kCompute, path, sourceCode);

    if (compiledCode.has_value()) {
        _cleanupShaderModules();
        _cachedShaderModule = _createShaderModule(compiledCode.value());
    } else {
        _logger->error("Failed to compile shader: {}", path);
        exit(0);
    }
}

// the shader module must be cached before this step
void ComputePipeline::build() {
    _cleanupPipelineAndLayout();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    // this is why the compute pipeline requires the descriptor set layout to be specified
    pipelineLayoutInfo.pSetLayouts = &_descriptorSetBundle->getDescriptorSetLayout();

    vkCreatePipelineLayout(_appContext->getDevice(), &pipelineLayoutInfo, nullptr,
                           &_pipelineLayout);

    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStageInfo.module = _cachedShaderModule;
    shaderStageInfo.pName  = "main"; // name of the entry function of current shader

    VkComputePipelineCreateInfo computePipelineCreateInfo{};
    computePipelineCreateInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineCreateInfo.layout = _pipelineLayout;
    computePipelineCreateInfo.flags  = 0;
    computePipelineCreateInfo.stage  = shaderStageInfo;

    vkCreateComputePipelines(_appContext->getDevice(), VK_NULL_HANDLE, 1,
                             &computePipelineCreateInfo, nullptr, &_pipeline);
}

void ComputePipeline::recordCommand(VkCommandBuffer commandBuffer, uint32_t currentFrame,
                                    uint32_t threadCountX, uint32_t threadCountY,
                                    uint32_t threadCountZ) {
    _bind(commandBuffer, currentFrame);
    vkCmdDispatch(commandBuffer,
                  static_cast<uint32_t>(std::ceil((float)threadCountX / (float)_workGroupSize.x)),
                  static_cast<uint32_t>(std::ceil((float)threadCountY / (float)_workGroupSize.y)),
                  static_cast<uint32_t>(std::ceil((float)threadCountZ / (float)_workGroupSize.z)));
}

void ComputePipeline::recordIndirectCommand(VkCommandBuffer commandBuffer, uint32_t currentFrame,
                                            VkBuffer indirectBuffer) {
    _bind(commandBuffer, currentFrame);
    vkCmdDispatchIndirect(commandBuffer, indirectBuffer, 0);
}

void ComputePipeline::_cleanupShaderModules() {
    if (_cachedShaderModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(_appContext->getDevice(), _cachedShaderModule, nullptr);
        _cachedShaderModule = VK_NULL_HANDLE;
    }
}
