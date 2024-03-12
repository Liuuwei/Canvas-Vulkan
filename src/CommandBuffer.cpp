#include "CommandBuffer.h"
#include "vulkan/vulkan_core.h"
#include <stdexcept>

CommandBuffer::CommandBuffer(VkDevice device, VkCommandBufferAllocateInfo commandBufferInfo) : device_(device) {
    if (vkAllocateCommandBuffers(device, &commandBufferInfo, &commandBuffer_) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffer!");
    }
}