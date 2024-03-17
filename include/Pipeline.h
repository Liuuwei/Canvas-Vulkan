#pragma once

#include "vulkan/vulkan_core.h"
#include <vulkan/vulkan.h>
#include <vector>

class  Pipeline {
public:
    Pipeline(VkDevice device, VkPipelineCache pipelineCache, const std::vector<VkGraphicsPipelineCreateInfo>& createInfos);
    ~Pipeline();

    VkPipeline get() const { return pipeline_; }
private:
    VkDevice device_;
    VkPipeline pipeline_;
};