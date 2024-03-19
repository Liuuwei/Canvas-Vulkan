#pragma once 

#include "vulkan/vulkan_core.h"
#include <vulkan/vulkan.h>

class PipelineLayout {
public:
    PipelineLayout(VkDevice device);
    ~PipelineLayout();

    void init();
    VkPipelineLayout pipelineLayout() const { return pipelineLayout_; }
    void* pNext_{};
    VkPipelineLayoutCreateFlags flags_{};
    uint32_t setLayoutCount_{};
    VkDescriptorSetLayout* pSetLayouts_{};
    uint32_t pushConstantRangeCount_{};
    VkPushConstantRange* pPushConstantRanges_{};
private:
    VkDevice device_;
    VkPipelineLayout pipelineLayout_;
};