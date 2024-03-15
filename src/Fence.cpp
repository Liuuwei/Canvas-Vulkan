#include "Fence.h"
#include "vulkan/vulkan_core.h"
#include <stdexcept>

Fence::Fence(VkDevice device, VkFenceCreateInfo createInfo) : device_(device) {
    if (vkCreateFence(device, &createInfo, nullptr, &fence_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create fence!");
    }
}