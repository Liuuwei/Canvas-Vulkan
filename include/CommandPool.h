#pragma once

#include "vulkan/vulkan_core.h"
#include <vulkan/vulkan.h>

class CommandPool {
public:
    CommandPool(VkDevice device, VkCommandPoolCreateInfo commandPoolInfo);
    ~CommandPool();

    VkCommandPool get() const { return commandPool_; }
private:
    VkDevice device_;
    VkCommandPool commandPool_;
};