#include "Memory.h"
#include "vulkan/vulkan_core.h"
#include <stdexcept>

Memory::Memory(VkDevice device, VkMemoryAllocateInfo memoryInfo) : device_(device) {
    if (vkAllocateMemory(device, &memoryInfo, nullptr, &memory_) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate device memory!");
    }
}

Memory::~Memory() {
    vkFreeMemory(device_, memory_, nullptr);
}