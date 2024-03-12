#include "Pipeline.h"
#include "vulkan/vulkan_core.h"
#include <stdexcept>

Pipeline::Pipeline(VkDevice device, VkPipelineCache pipelineCache, const std::vector<VkGraphicsPipelineCreateInfo>& createInfos) : device_(device) {
    if (vkCreateGraphicsPipelines(device, pipelineCache, createInfos.size(), createInfos.data(), nullptr, &pipeline_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }
}