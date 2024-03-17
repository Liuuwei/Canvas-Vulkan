#include "DescriptorPool.h"
#include "vulkan/vulkan_core.h"
#include <stdexcept>

DescriptorPool::DescriptorPool(VkDevice device, VkDescriptorPoolCreateInfo createInfo) : device_(device) {
    if (vkCreateDescriptorPool(device, &createInfo, nullptr, &descriptorPool_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool!");
    }
}

DescriptorPool::~DescriptorPool() {
    vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
}