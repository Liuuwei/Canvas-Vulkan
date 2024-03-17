#pragma once

#include "vulkan/vulkan_core.h"
#include <vulkan/vulkan.h>

class Semaphore {
public:
    Semaphore(VkDevice device, VkSemaphoreCreateInfo createInfo);
    ~Semaphore();

    VkSemaphore get() const { return semaphore_; }
    VkSemaphore* getPtr() { return &semaphore_; }
private:
    VkDevice device_;
    VkSemaphore semaphore_;
};