#include "ImageView.h"
#include "vulkan/vulkan_core.h"
#include <stdexcept>

ImageView::ImageView(VkDevice device, VkImageViewCreateInfo viewInfo) : device_(device) {
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image view!");
    }
}

ImageView::~ImageView() {
    vkDestroyImageView(device_, imageView_, nullptr);
}