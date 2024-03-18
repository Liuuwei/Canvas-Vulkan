#pragma once

#include "vulkan/vulkan_core.h"
#include <vulkan/vulkan.h>
#include <vector>

class  Pipeline {
public:
    Pipeline(VkDevice device, VkPipelineCache pipelineCache, const std::vector<VkGraphicsPipelineCreateInfo>& createInfos);
    ~Pipeline();

    void init();
    VkPipeline pipeline() const { return pipeline_; }
private:
    VkDevice device_;
    VkPipeline pipeline_;
    VkPipelineCreateFlags                            flags_{};
    uint32_t                                         stageCount_{};
    VkPipelineShaderStageCreateInfo*           pStages_{};
    VkPipelineVertexInputStateCreateInfo*      pVertexInputState_{};
    VkPipelineInputAssemblyStateCreateInfo*    pInputAssemblyState_{};
    VkPipelineTessellationStateCreateInfo*     pTessellationState_{};
    VkPipelineViewportStateCreateInfo*         pViewportState_{};
    VkPipelineRasterizationStateCreateInfo*    pRasterizationState_{};
    VkPipelineMultisampleStateCreateInfo*      pMultisampleState_{};
    VkPipelineDepthStencilStateCreateInfo*     pDepthStencilState_{};
    VkPipelineColorBlendStateCreateInfo*       pColorBlendState_{};
    VkPipelineDynamicStateCreateInfo*          pDynamicStat_{};
    VkPipelineLayout                                 layout_{};
    VkRenderPass                                     renderPass_{};
    uint32_t                                         subpass_{};
    VkPipeline                                       basePipelineHandle_{};
    int32_t                                          basePipelineIndex_{};
};