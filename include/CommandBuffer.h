#pragma once

#include "vulkan/vulkan_core.h"
class CommandBuffer {
public:
    CommandBuffer(VkDevice device, VkCommandBufferAllocateInfo commandBufferInfo);

    VkCommandBuffer get() const { return commandBuffer_; }
private:
    VkDevice device_;
    VkCommandBuffer commandBuffer_;
};