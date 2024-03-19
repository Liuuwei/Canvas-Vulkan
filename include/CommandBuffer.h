#pragma once

#include "vulkan/vulkan_core.h"
class CommandBuffer {
public:
    CommandBuffer(VkDevice device);
    ~CommandBuffer();

    void init();
    VkCommandBuffer commandBuffer() const { return commandBuffer_; }
    void*             pNext_{};
    VkCommandPool           commandPool_{};
    VkCommandBufferLevel    level_{};
    uint32_t                commandBufferCount_{};
private:
    VkDevice device_;
    VkCommandBuffer commandBuffer_;
};