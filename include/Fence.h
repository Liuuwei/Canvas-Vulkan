#pragma once

#include "vulkan/vulkan_core.h"
#include <vulkan/vulkan.h>

class Fence {
public:
    Fence(VkDevice device);
    ~Fence();

    void init();
    VkFence fence() const { return fence_; }
    VkFence* fencePtr() { return &fence_; }
    void*           pNext_{};
    VkFenceCreateFlags    flags_{};
private:
    VkDevice device_;
    VkFence fence_;
};