#pragma once

#include "vulkan/vulkan_core.h"
#include <vulkan/vulkan.h>

class Semaphore {
public:
    Semaphore(VkDevice device);
    ~Semaphore();

    void init();
    VkSemaphore semaphore() const { return semaphore_; }
    VkSemaphore* semaphorePtr() { return &semaphore_; }
    void*               pNext_{};
    VkSemaphoreCreateFlags    flags_{};
private:
    VkDevice device_;
    VkSemaphore semaphore_;
};