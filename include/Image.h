#pragma once

#include "vulkan/vulkan_core.h"
#include <vulkan/vulkan.h>

class Image {
public:
    Image(VkDevice device, VkImageCreateInfo imageInfo);
    
    VkImage get() const { return image_; }
    VkFormat format() const { return format_; }
private:
    VkDevice device_;
    VkImage image_;
    VkFormat format_;
};