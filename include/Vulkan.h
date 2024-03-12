#pragma once

#include "Image.h"
#include "ImageView.h"
#include "Memory.h"
#include "PipelineLayout.h"
#include "RenderPass.h"
#include "Swapchain.h"
#include "Vertex.h"
#include "vulkan/vulkan_core.h"
#include <cstdint>
#include <functional>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <vector>
#include <memory>

#include "VulkanHelp.h"
#include "DescriptorSetLayout.h"
#include "RenderPass.h"
#include "Pipeline.h"
#include "Swapchain.h"
#include "Memory.h"
#include "FrameBuffer.h"
#include "CommandPool.h"
#include "CommandBuffer.h"
#include "DescriptorPool.h"

class Vulkan {
public:
    Vulkan(const std::string& title, uint32_t width, uint32_t height);

    void run();
private:
    void initWindow();
    void initVulkan();

    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicDevice();
    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    void createDescriptorPool();
    void createDescriptorSetLayout();
    void createDescriptorSet();
    void createGraphicsPipelines();
    void createColorResource();
    void createDepthResource();
    void createFrameBuffer();
    void createCommandPool();
    void createCommandBuffers();
    void createVertexBuffer();
    void createIndexBuffer();

private:
    bool checkValidationLayerSupport() const;
    bool checkDeviceExtensionsSupport(VkPhysicalDevice physicalDevice) const;
    bool deviceSuitable(VkPhysicalDevice);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) const;
    VkSampleCountFlagBits getMaxUsableSampleCount() const;
    VkFormat findDepthFormat() const;
    VkFormat findSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags features) const;
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
    void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) const;
    VkCommandBuffer beginSingleTimeCommands() const;
    void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue queue) const;
    
    GLFWwindow* windows_;
    uint32_t width_;
    uint32_t height_;
    std::string title_;

    VkInstance instance_;
    VkDebugUtilsMessengerEXT debugMessenger_;
    VkSurfaceKHR surface_;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_;
    std::unique_ptr<RenderPass> renderPass_;
    VkQueue graphicsQueue_;
    VkQueue presentQueue_;
    VkQueue transferQueue_;

    std::function<void(VkImage*)> vkImageDelete;

    std::unique_ptr<SwapChain> swapChain_;

    std::unique_ptr<DescriptorPool> descriptorPool_;
    std::unique_ptr<DescriptorSetlayout> descriptorSetLayout_;
    std::vector<VkDescriptorSet> descriptorSets_;

    std::unique_ptr<PipelineLayout> pipelineLayout_;
    std::unique_ptr<Pipeline> graphicsPipeline_;

    std::unique_ptr<Image> colorImage_;
    std::unique_ptr<ImageView> colorImageView_;
    std::unique_ptr<Memory> colorImageMemory_;

    std::unique_ptr<Image> depthImage_;
    std::unique_ptr<ImageView> depthImageView_;

    std::vector<std::unique_ptr<FrameBuffer>> frameBuffers_;

    std::unique_ptr<CommandPool> commandPool_;
    std::vector<std::unique_ptr<CommandBuffer>> commandBuffers_;
    
    VulkanHelp::QueueFamilyIndices queueFamilies_;
    VkSampleCountFlagBits msaaSamples_ = VK_SAMPLE_COUNT_1_BIT;

    const std::vector<const char*> validationLayers_ = {"VK_LAYER_KHRONOS_validation"};
    const std::vector<const char*> deviceExtensions_ = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    std::unique_ptr<Vertex> vertex_;

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
        VkDebugUtilsMessageTypeFlagsEXT messageType, 
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
        void* pUserData
    );

    const int MAX_FRAMES_IN_FLIGHT = 2;
};