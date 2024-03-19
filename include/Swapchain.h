#pragma once

#include "vulkan/vulkan_core.h"
#include <cstddef>
#include <cstdint>
#include <vulkan/vulkan.h>

#include <vector>

class SwapChain {
public:
    SwapChain(VkDevice device);
    ~SwapChain();

    void init();
    VkSwapchainKHR swapChain() const { return swapChain_; }
    size_t size() const { return images_.size(); }
    VkImage image(uint32_t idx) const { return images_[idx]; }
    VkImageView imageView(uint32_t idx) const { return imageViews_[idx]; }
    VkFormat format() const { return format_; }
    VkExtent2D extent() const { return extent_; }
    uint32_t width() const { return extent_.width; }
    uint32_t height() const { return extent_.height; }
    void*                      pNext_{};
    VkSwapchainCreateFlagsKHR        flags_{};
    VkSurfaceKHR                     surface_{};
    uint32_t                         minImageCount_{};
    VkFormat                         imageFormat_{};
    VkColorSpaceKHR                  imageColorSpace_{};
    VkExtent2D                       imageExtent_{};
    uint32_t                         imageArrayLayers_{};
    VkImageUsageFlags                imageUsage_{};
    VkSharingMode                    imageSharingMode_{};
    uint32_t                         queueFamilyIndexCount_{};
    uint32_t*                  pQueueFamilyIndices_{};
    VkSurfaceTransformFlagBitsKHR    preTransform_{};
    VkCompositeAlphaFlagBitsKHR      compositeAlpha_{};
    VkPresentModeKHR                 presentMode_{};
    VkBool32                         clipped_{};
    VkSwapchainKHR                   oldSwapchain_{};
private:
    VkDevice device_;
    VkSwapchainKHR swapChain_;
    VkFormat format_;
    VkExtent2D extent_;
    std::vector<VkImage> images_;
    std::vector<VkImageView> imageViews_;
};