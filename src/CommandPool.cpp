#include "CommandPool.h"
#include "vulkan/vulkan_core.h"
#include <stdexcept>

CommandPool::CommandPool(VkDevice device, VkCommandPoolCreateInfo commandPoolInfo) : device_(device) {
    if (vkCreateCommandPool(device, &commandPoolInfo, nullptr, &commandPool_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}