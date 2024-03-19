#pragma once

#include "vulkan/vulkan_core.h"
#include <vulkan/vulkan.h>

class CommandPool {
public:
    CommandPool(VkDevice device);
    ~CommandPool();

    void init();
    VkCommandPool commanddPool() const { return commandPool_; }
    void*                 pNext_{};
    VkCommandPoolCreateFlags    flags_{};
    uint32_t                    queueFamilyIndex_{};
private:
    VkDevice device_;
    VkCommandPool commandPool_;
};