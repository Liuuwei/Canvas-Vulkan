#include "PipelineLayout.h"
#include "vulkan/vulkan_core.h"
#include "Tools.h"

PipelineLayout::PipelineLayout(VkDevice device) : device_(device) {

}

PipelineLayout::~PipelineLayout() {
    vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
}

void PipelineLayout::init() {
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.flags = flags_;
    pipelineLayoutInfo.pNext = pNext_;
    pipelineLayoutInfo.setLayoutCount = setLayoutCount_;
    pipelineLayoutInfo.pSetLayouts = pSetLayouts_;
    pipelineLayoutInfo.pushConstantRangeCount = pushConstantRangeCount_;
    pipelineLayoutInfo.pPushConstantRanges = pPushConstantRanges_;
    VK_CHECK(vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &pipelineLayout_));
}