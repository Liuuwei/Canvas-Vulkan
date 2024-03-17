#pragma once

#include "vulkan/vulkan_core.h"
#include <cstdint>
#include <vulkan/vulkan.h>

class Image {
public:
    Image(VkPhysicalDevice physicalDevice, VkDevice device);
    Image& operator=(const Image& rhs) = default;
    ~Image();
    
    void init();
    VkImage image() const { return image_; }
    VkImageView view() const { return view_; }
    VkDeviceMemory memory() const { return memory_; }
    
private:
    VkPhysicalDevice physicalDevice_;
    VkDevice device_;
    VkImage image_;
    VkImageView view_;
    VkDeviceMemory memory_;
    
public:
    VkImageType imageType_;
    VkImageCreateFlags flags_ = 0;
    VkFormat format_;
    VkExtent3D extent_;
    uint32_t mipLevles_;
    uint32_t arrayLayers_;
    VkSampleCountFlagBits samples_;
    VkImageTiling tiling_;
    VkImageUsageFlags usage_;
    VkSharingMode sharingMode_;
    uint32_t queueFamilyIndexCount_;
    const uint32_t* pQueueFamilyIndices_;    

    VkMemoryPropertyFlags memoryProperties_;

    VkImageViewType viewType_;
    VkImageSubresourceRange subresourcesRange_;
};