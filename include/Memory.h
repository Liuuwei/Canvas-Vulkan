#pragma once

#include "vulkan/vulkan_core.h"

class Memory {
public:
    Memory(VkDevice device, VkMemoryAllocateInfo memoryInfo);

    VkDeviceMemory get() const { return memory_; }
private:
    VkDevice device_;
    VkDeviceMemory memory_;
};