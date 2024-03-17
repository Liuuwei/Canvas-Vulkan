#include "PipelineLayout.h"
#include "vulkan/vulkan_core.h"
#include <stdexcept>

PipelineLayout::PipelineLayout(VkDevice device, VkPipelineLayoutCreateInfo layoutInfo) : device_(device) {
    if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }
}

PipelineLayout::~PipelineLayout() {
    vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
}