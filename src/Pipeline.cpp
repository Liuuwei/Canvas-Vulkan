#include "Pipeline.h"
#include "vulkan/vulkan_core.h"
#include "Tools.h"
#include <stdexcept>

Pipeline::Pipeline(VkDevice device) : device_(device) {

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
    pipelineInfo.pMultisampleState = pMultisampleState_;
    pipelineInfo.pDepthStencilState = pDepthStencilState_;
    pipelineInfo.pColorBlendState = pColorBlendState_;
    pipelineInfo.pDynamicState = pDynamicState_;
    pipelineInfo.layout = layout_;
    pipelineInfo.renderPass = renderPass_;
    pipelineInfo.subpass = subpass_;
    pipelineInfo.basePipelineHandle = basePipelineHandle_;
    pipelineInfo.basePipelineIndex = basePipelineIndex_;
    VK_CHECK(vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline_));
}