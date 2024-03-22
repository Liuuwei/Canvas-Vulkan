#pragma once

#include "GLFW/glfw3.h"
#include "vulkan/vulkan_core.h"
#include <cmath>
#include <glm/glm.hpp>
#include <algorithm>
#include <limits>
#include <stdexcept>

#include <vector>
#include <string>
#include <set>
#include <optional>
#include <fstream>
#include <iostream>
#include <cassert>

struct Tools {

struct QueueFamilyIndices {
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;
    std::optional<uint32_t> transfer;

    bool compeleted() const {
        return graphics.has_value() && present.has_value() && transfer.has_value();
    }

    bool multiple() const {
        return graphics.value() != present.value() || present.value() != transfer.value();
    }

    std::vector<uint32_t> sets() const {
        std::set<uint32_t> unique{graphics.value(), present.value(), transfer.value()};
        return {unique.begin(), unique.end()};
    }

    VkSharingMode sharingMode() const {
        return multiple() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    }

    static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
        QueueFamilyIndices indices;

        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(count);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, queueFamilies.data());

        for (size_t i = 0; i < queueFamilies.size(); i++) {
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphics = i;
            }
            if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
                indices.transfer = i;
            }

            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
            if (presentSupport) {
                indices.present = i;
            }

            if (indices.compeleted()) {
                break;
            }
        }

        return indices;
    }
};

struct SwapChainSupportDetail {
    VkSurfaceCapabilitiesKHR capabilities_;
    std::vector<VkSurfaceFormatKHR> formats_;
    std::vector<VkPresentModeKHR> presentModes_;

    VkSurfaceFormatKHR chooseSurfaceFormat() const {
        for (const auto& format : formats_) {
            if (format.format == VK_FORMAT_R8G8B8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return format;
            }
        }

        return *formats_.begin();
    }

    VkExtent2D chooseSwapExtent(GLFWwindow* window) const {
        if (capabilities_.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities_.currentExtent;
        } else {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width), 
                static_cast<uint32_t>(height), 
            };

            actualExtent.width = std::clamp(actualExtent.width, capabilities_.minImageExtent.width, capabilities_.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities_.minImageExtent.height, capabilities_.maxImageExtent.height);

            return actualExtent;
        }
    }

    VkPresentModeKHR choosePresentMode() const {
        for (const auto& mode : presentModes_) {
            if (mode == VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR) {
                return mode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    static SwapChainSupportDetail querySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
        SwapChainSupportDetail detail;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &detail.capabilities_);

        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
        if (formatCount != 0) {
            detail.formats_.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, detail.formats_.data());
        }

        uint32_t presentCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentCount, nullptr);
        if (presentCount != 0) {
            detail.presentModes_.resize(presentCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentCount, detail.presentModes_.data());
        }

        return detail;
    }
};

static std::vector<const char*> getRequiredExtensions() {
    uint32_t count = 0;
    auto extensions = glfwGetRequiredInstanceExtensions(&count);

    std::vector<const char*> requiredExtensions(extensions, extensions + count);
    requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    requiredExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    requiredExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    return requiredExtensions;
}   

static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo, PFN_vkDebugUtilsMessengerCallbackEXT callback) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = callback;
}

static VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

static std::vector<char> readFile(const std::string& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error(std::string("failed to open file: " + path).c_str());
    }

    size_t fileSize = static_cast<uint32_t>(file.tellg());
    std::vector<char> buffer(fileSize);
    
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    
    for (size_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & 1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

static void setImageLayout(
    VkCommandBuffer cmdBuffer, 
    VkImage image, 
    VkImageLayout oldLayout, 
    VkImageLayout newLayout, 
    VkImageSubresourceRange subresourcesRange, 
    VkPipelineStageFlags srcStageMask, 
    VkPipelineStageFlags dstStageMask) {
    VkImageMemoryBarrier imageMemoryBarrier{};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.oldLayout = oldLayout;
    imageMemoryBarrier.newLayout = newLayout;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.subresourceRange = subresourcesRange;

    switch (oldLayout) {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        imageMemoryBarrier.srcAccessMask = 0;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

    default:
        throw std::runtime_error("unknown old layout!");
    }

    switch (newLayout) {
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break; 
    
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        if (imageMemoryBarrier.srcAccessMask == 0) {
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        }
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;

    default:
        throw std::runtime_error("unknown new layout!");
    }

    vkCmdPipelineBarrier(
        cmdBuffer, 
        srcStageMask, dstStageMask, 
        0, 
        0, nullptr, 
        0, nullptr, 
        1, &imageMemoryBarrier
    );
}

static std::string errorString(VkResult errorCode)
{
    switch (errorCode)
    {
#define STR(r) case VK_ ##r: return #r
        STR(NOT_READY);
        STR(TIMEOUT);
        STR(EVENT_SET);
        STR(EVENT_RESET);
        STR(INCOMPLETE);
        STR(ERROR_OUT_OF_HOST_MEMORY);
        STR(ERROR_OUT_OF_DEVICE_MEMORY);
        STR(ERROR_INITIALIZATION_FAILED);
        STR(ERROR_DEVICE_LOST);
        STR(ERROR_MEMORY_MAP_FAILED);
        STR(ERROR_LAYER_NOT_PRESENT);
        STR(ERROR_EXTENSION_NOT_PRESENT);
        STR(ERROR_FEATURE_NOT_PRESENT);
        STR(ERROR_INCOMPATIBLE_DRIVER);
        STR(ERROR_TOO_MANY_OBJECTS);
        STR(ERROR_FORMAT_NOT_SUPPORTED);
        STR(ERROR_SURFACE_LOST_KHR);
        STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
        STR(SUBOPTIMAL_KHR);
        STR(ERROR_OUT_OF_DATE_KHR);
        STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
        STR(ERROR_VALIDATION_FAILED_EXT);
        STR(ERROR_INVALID_SHADER_NV);
        STR(ERROR_INCOMPATIBLE_SHADER_BINARY_EXT);
#undef STR
    default:
        return "UNKNOWN_ERROR";
    }
}

#ifndef VK_CHECK
#define VK_CHECK(f) \
{ \
    VkResult res = (f); \
    if (res != VK_SUCCESS) { \
        std::cout << "Fatal: VkResult is " << Tools::errorString(res) << __FILE__ << " at line " << __LINE__ << std::endl; \
        assert(res == VK_SUCCESS); \
    } \
}

#endif

static float pointToLineLength(float a, float b, float x, float y) {
    // y = ax + b; length = sin * r; r = x1 - x2; sin = abs(tan) / sqrt(1 + tan2)
    float x1;
    if (a == 0) {
        x1 = x;
    } else {
        x1 = (y - b) / a;
    }
    auto r = std::abs(x - x1);
    auto tan = a;
    auto sin = std::abs(tan) / std::sqrt(1 + std::pow(tan, 2));

    return sin * r;
}

static char keyToChar(int key) {
    switch (key) {
    case GLFW_KEY_A:
        return 'a';
    case GLFW_KEY_B:
        return 'b';
    case GLFW_KEY_C:
        return 'c';
    case GLFW_KEY_D:
        return 'd';
    case GLFW_KEY_E:
        return 'e';
    case GLFW_KEY_F:
        return 'f';
    case GLFW_KEY_G:
        return 'g';
    case GLFW_KEY_H:
        return 'h';
    case GLFW_KEY_I:
        return 'i';
    case GLFW_KEY_J:
        return 'j';
    case GLFW_KEY_K:
        return 'k';
    case GLFW_KEY_L:
        return 'l';
    case GLFW_KEY_M:
        return 'm';
    case GLFW_KEY_N:
        return 'n';
    case GLFW_KEY_O:
        return 'o';
    case GLFW_KEY_P:
        return 'p';
    case GLFW_KEY_Q:
        return 'q';
    case GLFW_KEY_R:
        return 'r';
    case GLFW_KEY_S:
        return 's';
    case GLFW_KEY_T:
        return 't';
    case GLFW_KEY_U:
        return 'u';
    case GLFW_KEY_V:
        return 'v';
    case GLFW_KEY_W:
        return 'w';
    case GLFW_KEY_X:
        return 'x';
    case GLFW_KEY_Y:
        return 'y';
    case GLFW_KEY_Z:
        return 'z';
    }

    return ' ';
}

};