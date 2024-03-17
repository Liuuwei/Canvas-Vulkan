#pragma once

#include "vulkan/vulkan_core.h"

class ImageView {
public:
    ImageView(VkDevice device, VkImageViewCreateInfo viewInfo);
    ~ImageView();

    VkImageView get() const { return imageView_; }
private:
    VkDevice device_;
    VkImageView imageView_;
};