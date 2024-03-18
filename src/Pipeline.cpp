#include "Pipeline.h"
#include "vulkan/vulkan_core.h"
#include <stdexcept>

Pipeline::Pipeline(VkDevice device, VkPipelineCache pipelineCache, const std::vector<VkGraphicsPipelineCreateInfo>& createInfos) : device_(device) {
    if (vkCreateGraphicsPipelines(device, pipelineCache, createInfos.size(), createInfos.data(), nullptr, &pipeline_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }
}

Pipeline::~Pipeline() {
    vkDestroyPipeline(device_, pipeline_, nullptr);
}

void Pipeline::init() {
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.flags = flags_;
    pipelineInfo.stageCount = stageCount_;
    pipelineInfo.pStages = pStages_;
    pipelineInfo.pVertexInputState = pVertexInputState_;
    pipelineInfo.pInputAssemblyState = pInputAssemblyState_;
    pipelineInfo.pTessellationState = pTessellationState_;
    pipelineInfo.pViewportState = pViewportState_;
    pipelineInfo.pRasterizationState = pRasterizationState_;
    
}