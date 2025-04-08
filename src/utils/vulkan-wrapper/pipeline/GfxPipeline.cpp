#include "GfxPipeline.hpp"

#include "app-context/VulkanApplicationContext.hpp"

#include "../descriptor-set/DescriptorSetBundle.hpp"
#include "utils/io/FileReader.hpp"
#include "utils/logger/Logger.hpp"
#include "utils/vulkan-wrapper/memory/Model.hpp"
#include "utils/shader-compiler/ShaderCompiler.hpp"

GfxPipeline::GfxPipeline(VulkanApplicationContext *appContext, Logger *logger, WorkGroupSize workGroupSize, DescriptorSetBundle *descriptorSetBundle, ShaderCompiler *shaderCompiler)
    : Pipeline(appContext, logger, descriptorSetBundle, VK_SHADER_STAGE_COMPUTE_BIT), _workGroupSize(workGroupSize), _shaderCompiler(shaderCompiler) { }

GfxPipeline::~GfxPipeline() = default;

void GfxPipeline::compileAndCacheShaderModule(std::string &path) {
  auto const sourceVertCode = FileReader::readShaderSourceCode(path + "/vert.glsl", _logger);
  auto const compiledVertCode = _shaderCompiler->compileShaderFromFile(ShaderStage::kVert, path, sourceVertCode);

  auto const sourceFragCode = FileReader::readShaderSourceCode(path + "/frag.glsl", _logger);
  auto const compiledFragCode = _shaderCompiler->compileShaderFromFile(ShaderStage::kFrag, path, sourceFragCode);

  if (compiledVertCode.has_value() && compiledFragCode.has_value()) {
    _cleanupShaderModule();
    _vertShaderModule = _createShaderModule(compiledVertCode.value());
    _fragShaderModule = _createShaderModule(compiledFragCode.value());
  } else {
    _logger->error("Failed to compile vert shader: {}", path);
    exit(0);
  }
}

void GfxPipeline::build() {
  _cleanupPipelineAndLayout();

  auto bindingDesc = Vertex::GetBindingDescription();
  auto attrDesc = Vertex::GetAttributeDescriptions();
  VkPipelineVertexInputStateCreateInfo vertInputInfo{};
  vertInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertInputInfo.vertexBindingDescriptionCount = 1;
  vertInputInfo.pVertexBindingDescriptions = &bindingDesc;
  vertInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDesc.size());
  vertInputInfo.pVertexAttributeDescriptions = attrDesc.data();

  std::vector<VkDynamicState> dynamicStates = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
  };
  VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
  dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  dynamicStateInfo.pDynamicStates = dynamicStates.data();

  VkPipelineInputAssemblyStateCreateInfo assemblyStateInfo{};
  assemblyStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  assemblyStateInfo.flags = 0;
  assemblyStateInfo.primitiveRestartEnable = VK_FALSE;
  assemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

  VkPipelineRasterizationStateCreateInfo rasterizeInfo{};
  rasterizeInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizeInfo.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizeInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
  rasterizeInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizeInfo.flags = 0;
  rasterizeInfo.depthClampEnable = VK_FALSE;
  rasterizeInfo.lineWidth = 1.0f;
  rasterizeInfo.depthBiasEnable = VK_FALSE;
  rasterizeInfo.rasterizerDiscardEnable = VK_FALSE;
  rasterizeInfo.depthBiasConstantFactor = 0.0f;
  rasterizeInfo.depthBiasClamp = 0.0f;
  rasterizeInfo.depthBiasSlopeFactor = 0.0f;

  VkPipelineColorBlendAttachmentState attachmentState{};
  attachmentState.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  attachmentState.blendEnable = VK_TRUE;
  attachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  attachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  attachmentState.colorBlendOp = VK_BLEND_OP_ADD;
  attachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  attachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  attachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo blendState{};
  blendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  blendState.attachmentCount = 1;
  blendState.pAttachments = &attachmentState;

  VkPipelineDepthStencilStateCreateInfo depthStencilState{};
  depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencilState.depthTestEnable = VK_TRUE;
  depthStencilState.depthWriteEnable = VK_TRUE;
  depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
  depthStencilState.depthBoundsTestEnable = VK_FALSE;
  depthStencilState.stencilTestEnable = VK_FALSE;
  depthStencilState.minDepthBounds = 0.0f;
  depthStencilState.maxDepthBounds = 1.0f;
  depthStencilState.stencilTestEnable = VK_FALSE;
  depthStencilState.front = {};
  depthStencilState.back = {};

  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;
  viewportState.flags = 0;

  VkPipelineMultisampleStateCreateInfo multisampleState{};
  multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampleState.sampleShadingEnable = VK_FALSE;
  multisampleState.rasterizationSamples = _appContext->getMsaaSample();
  multisampleState.minSampleShading = 1.0f;
  multisampleState.pSampleMask = nullptr;
  multisampleState.alphaToCoverageEnable = VK_FALSE;
  multisampleState.alphaToOneEnable = VK_FALSE;

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &_descriptorSetBundle->getDescriptorSetLayout();
  vkCreatePipelineLayout(_appContext->getDevice(), &pipelineLayoutInfo, nullptr, &_pipelineLayout);

  std::array<VkPipelineShaderStageCreateInfo, 2> shaderStageInfos{};
  shaderStageInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStageInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  shaderStageInfos[0].module = _vertShaderModule;
  shaderStageInfos[0].pName = "main";
  shaderStageInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStageInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  shaderStageInfos[1].module = _fragShaderModule;
  shaderStageInfos[1].pName = "main";

  VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
  pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineCreateInfo.pInputAssemblyState = &assemblyStateInfo;
  pipelineCreateInfo.pViewportState = &viewportState;
  pipelineCreateInfo.pRasterizationState = &rasterizeInfo;
  pipelineCreateInfo.pMultisampleState = &multisampleState;
  pipelineCreateInfo.pDepthStencilState = &depthStencilState;
  pipelineCreateInfo.pColorBlendState = &blendState;
  pipelineCreateInfo.pDynamicState = &dynamicStateInfo;
  pipelineCreateInfo.layout = _pipelineLayout;
  pipelineCreateInfo.renderPass = _renderPass;
  pipelineCreateInfo.subpass = 0;
  pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineCreateInfo.basePipelineIndex = -1;
  pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStageInfos.size());
  pipelineCreateInfo.pStages = shaderStageInfos.data();
  pipelineCreateInfo.pVertexInputState = &vertInputInfo;
  vkCreateGraphicsPipelines(_appContext->getDevice(), _pipelineCache, 1, & pipelineCreateInfo, nullptr, &_pipeline);
}

void GfxPipeline::recordCommand(VkCommandBuffer commandBuffer, uint32_t currentFrame,
                                    uint32_t threadCountX, uint32_t threadCountY,
                                    uint32_t threadCountZ) {
  _bind(commandBuffer, currentFrame);
  vkCmdDispatch(commandBuffer,
                static_cast<uint32_t>(std::ceil((float)threadCountX / (float)_workGroupSize.x)),
                static_cast<uint32_t>(std::ceil((float)threadCountY / (float)_workGroupSize.y)),
                static_cast<uint32_t>(std::ceil((float)threadCountZ / (float)_workGroupSize.z)));
}

void GfxPipeline::recordIndirectCommand(VkCommandBuffer commandBuffer, uint32_t currentFrame,
                                            VkBuffer indirectBuffer) {
  _bind(commandBuffer, currentFrame);
  vkCmdDispatchIndirect(commandBuffer, indirectBuffer, 0);
}

void GfxPipeline::_cleanupShaderModule() {
  if (_vertShaderModule != VK_NULL_HANDLE) {
    vkDestroyShaderModule(_appContext->getDevice(), _vertShaderModule, nullptr);
    _vertShaderModule = VK_NULL_HANDLE;
  }
  if (_fragShaderModule != VK_NULL_HANDLE) {
    vkDestroyShaderModule(_appContext->getDevice(), _fragShaderModule, nullptr);
    _fragShaderModule = VK_NULL_HANDLE;
  }
}