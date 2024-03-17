#pragma once

#include "vulkan/vulkan_core.h"
#include <vulkan/vulkan.h>

class Fence {
public:
    Fence(VkDevice device, VkFenceCreateInfo createInfo);
    ~Fence();

    VkFence get() const { return fence_; }
    VkFence* getPtr() { return &fence_; }
private:
    VkDevice device_;
    VkFence fence_;
};