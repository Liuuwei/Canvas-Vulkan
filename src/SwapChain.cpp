#include "Swapchain.h"
#include "vulkan/vulkan_core.h"
#include <cstddef>
#include <cstdint>
#include <stdexcept>

SwapChain::SwapChain(VkDevice device, VkSwapchainCreateInfoKHR swapChainInfo) : device_(device) {
    if (vkCreateSwapchainKHR(device, &swapChainInfo, nullptr, &swapChain_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    format_ = swapChainInfo.imageFormat;
    extent_ = swapChainInfo.imageExtent;

    uint32_t count = 0;
    vkGetSwapchainImagesKHR(device, swapChain_, &count, nullptr);
    images_.resize(count);
    imageViews_.resize(count);
    vkGetSwapchainImagesKHR(device, swapChain_, &count, images_.data());

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format_;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.layerCount = 1;
    viewInfo.subresourceRange.levelCount = 1;

    for (size_t i = 0; i < images_.size(); i++) {
        viewInfo.image = images_[i];
        if (vkCreateImageView(device, &viewInfo, nullptr, &imageViews_[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to creat swap chain image view!");
        }
    }
}