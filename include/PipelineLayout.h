#pragma once 

#include "vulkan/vulkan_core.h"
#include <vulkan/vulkan.h>

class PipelineLayout {
public:
    PipelineLayout(VkDevice device, VkPipelineLayoutCreateInfo layoutInfo);
    ~PipelineLayout();

    VkPipelineLayout get() const { return pipelineLayout_; }
private:
    VkDevice device_;
    VkPipelineLayout pipelineLayout_;
};