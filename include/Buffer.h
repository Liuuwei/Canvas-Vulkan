#pragma once

#include "vulkan/vulkan_core.h"
#include <cstdint>
#include <vulkan/vulkan.h>

class Buffer {
public:
    Buffer(VkPhysicalDevice physicalDevice, VkDevice device);
    ~Buffer();

    void init();

    VkBuffer buffer() const { return buffer_; }
    VkDeviceMemory memory() const { return memory_; }
    void* map(VkDeviceSize size) {
        if (data_) {
            return data_;
        }
        
        vkMapMemory(device_, memory_, 0, size, 0, &data_);
        return data_;
    }

    void unMap() {
        data_ = nullptr;
        vkUnmapMemory(device_, memory_);
    }

private:
    VkPhysicalDevice physicalDevice_;
    VkDevice device_;
    VkBuffer buffer_;
    VkDeviceMemory memory_;
    void* data_ = nullptr;
    
public:
    VkDeviceSize size_;
    VkBufferUsageFlags usage_;
    VkSharingMode sharingMode_;
    uint32_t queueFamilyIndexCount_;
    const uint32_t* pQueueFamilyIndices_;

    VkMemoryPropertyFlags memoryProperties_;
};