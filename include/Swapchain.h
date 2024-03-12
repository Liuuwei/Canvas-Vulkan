#pragma once

#include "vulkan/vulkan_core.h"
#include <cstddef>
#include <cstdint>
#include <vulkan/vulkan.h>

#include <vector>

class SwapChain {
public:
    SwapChain(VkDevice device, VkSwapchainCreateInfoKHR swapChainInfo);
    VkSwapchainKHR get() const { return swapChain_; }

    size_t size() const { return images_.size(); }
    VkImage image(uint32_t idx) const { return images_[idx]; }
    VkImageView imageView(uint32_t idx) const { return imageViews_[idx]; }
    VkFormat format() const { return format_; }
    VkExtent2D extent() const { return extent_; }
private:
    VkDevice device_;
    VkSwapchainKHR swapChain_;
    VkFormat format_;
    VkExtent2D extent_;
    std::vector<VkImage> images_;
    std::vector<VkImageView> imageViews_;
};