#include "CommandBuffer.h"
#include "vulkan/vulkan_core.h"
#include <stdexcept>
#include <iostream>

CommandBuffer::CommandBuffer(VkDevice device, VkCommandBufferAllocateInfo commandBufferInfo) : device_(device) {
    if (vkAllocateCommandBuffers(device, &commandBufferInfo, &commandBuffer_) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffer!");
    }
    commandPool_ = commandBufferInfo.commandPool;
}

CommandBuffer::~CommandBuffer() {
    vkFreeCommandBuffers(device_, commandPool_, 1, &commandBuffer_);
}