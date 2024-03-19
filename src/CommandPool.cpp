#include "CommandPool.h"
#include "vulkan/vulkan_core.h"
#include "Tools.h"

CommandPool::CommandPool(VkDevice device) : device_(device) {
    
}

CommandPool::~CommandPool() {
    vkDestroyCommandPool(device_, commandPool_, nullptr);
}

void CommandPool::init() {
    VkCommandPoolCreateInfo commandPoolInfo{};
    commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolInfo.flags = flags_;
    commandPoolInfo.pNext = pNext_;
    commandPoolInfo.queueFamilyIndex = queueFamilyIndex_;
    VK_CHECK(vkCreateCommandPool(device_, &commandPoolInfo, nullptr, &commandPool_));
}