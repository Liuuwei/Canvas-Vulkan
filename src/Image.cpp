#include "Image.h"
#include "vulkan/vulkan_core.h"
#include <stdexcept>

Image::Image(VkDevice device, VkImageCreateInfo imageInfo) : device_(device) {
    if (vkCreateImage(device, &imageInfo, nullptr, &image_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    format_ = imageInfo.format;
}