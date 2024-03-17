#pragma once

#include "vulkan/vulkan_core.h"
class CommandBuffer {
public:
    CommandBuffer(VkDevice device, VkCommandBufferAllocateInfo commandBufferInfo);
    ~CommandBuffer();

    VkCommandBuffer get() const { return commandBuffer_; }
private:
    VkDevice device_;
    VkCommandPool commandPool_;
    VkCommandBuffer commandBuffer_;
};