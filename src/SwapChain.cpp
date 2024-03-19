#include "Swapchain.h"
#include "Tools.h"
#include "vulkan/vulkan_core.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <stdexcept>

SwapChain::SwapChain(VkDevice device) : device_(device) {
    
}

SwapChain::~SwapChain() {
    vkDestroySwapchainKHR(device_, swapChain_, nullptr);
}

void SwapChain::init() {
    VkSwapchainCreateInfoKHR swapChainInfo{};
    swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainInfo.pNext = pNext_;
    swapChainInfo.flags = flags_;
    swapChainInfo.surface = surface_;
    swapChainInfo.minImageCount = minImageCount_;
    swapChainInfo.imageFormat = imageFormat_;
    swapChainInfo.imageColorSpace = imageColorSpace_;
    swapChainInfo.imageExtent = imageExtent_;
    swapChainInfo.imageArrayLayers = imageArrayLayers_;
    swapChainInfo.imageUsage = imageUsage_;
    swapChainInfo.imageSharingMode = imageSharingMode_;
    swapChainInfo.queueFamilyIndexCount = queueFamilyIndexCount_;
    swapChainInfo.pQueueFamilyIndices = pQueueFamilyIndices_;
    swapChainInfo.preTransform = preTransform_;
    swapChainInfo.compositeAlpha = compositeAlpha_;
    swapChainInfo.presentMode = presentMode_;
    swapChainInfo.clipped = clipped_;
    swapChainInfo.oldSwapchain = oldSwapchain_;
    VK_CHECK(vkCreateSwapchainKHR(device_, &swapChainInfo, nullptr, &swapChain_));

    format_ = swapChainInfo.imageFormat;
    extent_ = swapChainInfo.imageExtent;

    uint32_t count = 0;
    vkGetSwapchainImagesKHR(device_, swapChain_, &count, nullptr);
    images_.resize(count);
    imageViews_.resize(count);
    vkGetSwapchainImagesKHR(device_, swapChain_, &count, images_.data());

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format_;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.layerCount = 1;
    viewInfo.subresourceRange.levelCount = 1;

    for (size_t i = 0; i < images_.size(); i++) {
        viewInfo.image = images_[i];
        if (vkCreateImageView(device_, &viewInfo, nullptr, &imageViews_[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to creat swap chain image view!");
        }
    }
}