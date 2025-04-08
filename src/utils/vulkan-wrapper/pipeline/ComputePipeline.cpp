#include "ComputePipeline.hpp"

#include "app-context/VulkanApplicationContext.hpp"

#include "../descriptor-set/DescriptorSetBundle.hpp"
#include "utils/io/FileReader.hpp"
#include "utils/logger/Logger.hpp"
#include "utils/shader-compiler/ShaderCompiler.hpp"

ComputePipeline::ComputePipeline(VulkanApplicationContext *appContext, Logger *logger,
                                 std::string fullPathToShaderSourceCode,
                                 WorkGroupSize workGroupSize,
                                 DescriptorSetBundle *descriptorSetBundle,
                                 ShaderCompiler *shaderCompiler)
    : Pipeline(appContext, logger, std::move(fullPathToShaderSourceCode), descriptorSetBundle,
               VK_SHADER_STAGE_COMPUTE_BIT),
      _workGroupSize(workGroupSize), _shaderCompiler(shaderCompiler) {
  if (!compileAndCacheShaderModule()) {
    _logger->error("pipeline: {} is failed to compile!");
    exit(0);
  }
  build();
}

ComputePipeline::~ComputePipeline() = default;

bool ComputePipeline::compileAndCacheShaderModule() {
  auto const sourceCode =

      FileReader::readShaderSourceCode(_fullPathToShaderSourceCode, _logger);
  auto const compiledCode = _shaderCompiler->compileShaderFromFile(
      ShaderStage::kCompute, _fullPathToShaderSourceCode, sourceCode);

  if (compiledCode.has_value()) {
    _cleanupShaderModule();
    _cachedShaderModule = _createShaderModule(compiledCode.value());
    return true;
  }
  return false;
}

// the shader module must be cached before this step
void ComputePipeline::build() {
  _cleanupPipelineAndLayout();

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  // this is why the compute pipeline requires the descriptor set layout to be specified
  pipelineLayoutInfo.pSetLayouts = &_descriptorSetBundle->getDescriptorSetLayout();

  vkCreatePipelineLayout(_appContext->getDevice(), &pipelineLayoutInfo, nullptr, &_pipelineLayout);

  if (_cachedShaderModule == VK_NULL_HANDLE) {
    _logger->error("failed to build the pipeline because of a null shader module: {}",
                   _fullPathToShaderSourceCode);
  }

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

  vkCreateComputePipelines(_appContext->getDevice(), VK_NULL_HANDLE, 1, &computePipelineCreateInfo,
                           nullptr, &_pipeline);
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
