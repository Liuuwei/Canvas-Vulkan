#pragma once

#include "Buffer.h"
#include "Fence.h"
#include "Image.h"
#include "PipelineLayout.h"
#include "RenderPass.h"
#include "Sampler.h"
#include "Semaphore.h"
#include "Swapchain.h"
#include "Vertex.h"
#include "vulkan/vulkan_core.h"
#include <cstdint>
#include <functional>
#include <utility>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <vector>
#include <memory>

#include "Tools.h"
#include "DescriptorSetLayout.h"
#include "RenderPass.h"
#include "Pipeline.h"
#include "Swapchain.h"
#include "FrameBuffer.h"
#include "CommandPool.h"
#include "CommandBuffer.h"
#include "DescriptorPool.h"
#include "Block.h"
#include "Sampler.h"
#include "Camera.h"
#include "Timer.h"
#include "Font.h"
#include "Plane.h"

#include "TcpConnection.h"

class Vulkan {
public:
    Vulkan(const std::string& title, uint32_t width, uint32_t height);

    void run();
    void pollEvents();
    void draw();
    void processNetWork(const std::string& msg);
    std::shared_ptr<TcpConnection> tcpConnection_;
    GLFWwindow* windows_;
private:
    void initWindow();
    void initVulkan();

    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicDevice();
    void createSwapChain();
    void createRenderPass();
    void createUniformBuffers();
    void createSamplers();
    void createDescriptorPool();
    void createDescriptorSetLayout();
    void createDescriptorSet();
    void createVertex();
    void createGraphicsPipelines();
    void createColorResource();
    void createDepthResource();
    void createFrameBuffer();
    void createCommandPool();
    void createCommandBuffers();
    void createVertexBuffer();
    void createIndexBuffer();
    void createSyncObjects();
    void recordCommadBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void loadAssets();
    void updateDrawAssets();
    void recreateSwapChain();
    void loadChars();

private:
    bool checkValidationLayerSupport() ;
    bool checkDeviceExtensionsSupport(VkPhysicalDevice physicalDevice) ;
    bool deviceSuitable(VkPhysicalDevice);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) ;
    VkSampleCountFlagBits getMaxUsableSampleCount() ;
    VkFormat findDepthFormat() ;
    VkFormat findSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags features) ;
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) ;
    void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) ;
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue queue);
    void fillColor(std::vector<float>& vertices);
    std::pair<std::pair<float, float>, std::pair<float, float>> parseMsg(const std::string& msg);
    
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

    std::unique_ptr<Sampler> sampler_;

    std::unique_ptr<DescriptorPool> descriptorPool_;
    std::unique_ptr<DescriptorSetLayout> descriptorSetLayout_;
    std::vector<VkDescriptorSet> descriptorSets_;

    std::unique_ptr<PipelineLayout> pipelineLayout_;
    std::unique_ptr<Pipeline> graphicsPipeline_;

    std::unique_ptr<Image> colorImage_;

    std::unique_ptr<Image> depthImage_;

    std::vector<std::unique_ptr<FrameBuffer>> frameBuffers_;

    std::unique_ptr<CommandPool> commandPool_;
    std::vector<std::unique_ptr<CommandBuffer>> commandBuffers_;

    std::unique_ptr<myVK::Buffer> vertexBuffer_;
    std::unique_ptr<myVK::Buffer> indexBuffer_;

    std::vector<std::unique_ptr<Fence>> inFlightFences_;
    std::vector<std::unique_ptr<Semaphore>> imageAvaiableSemaphores_;
    std::vector<std::unique_ptr<Semaphore>> renderFinishSemaphores_;

    Tools::QueueFamilyIndices queueFamilies_;
    VkSampleCountFlagBits msaaSamples_ = VK_SAMPLE_COUNT_1_BIT;

    const std::vector<const char*> validationLayers_ = {"VK_LAYER_KHRONOS_validation"};
    const std::vector<const char*> deviceExtensions_ = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};


    uint32_t currentFrame_ = 0;

    std::vector<std::unique_ptr<myVK::Buffer>> uniformBuffers_;

    std::shared_ptr<Camera> camera_;
    Timer timer_;

    struct Character {
        Character() {}
        std::unique_ptr<Image> image_ = nullptr;
        uint32_t width_ = 0;
        uint32_t height_ = 0;
        uint32_t x_ = 0;
    };

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
        VkDebugUtilsMessageTypeFlagsEXT messageType, 
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
        void* pUserData
    );

    bool resized_ = false;

    static void frameBufferResizedCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<Vulkan*>(glfwGetWindowUserPointer(window));
        app->resized_ = true;
    }

    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    std::unique_ptr<Plane> vertex_;
    std::vector<float> vertices_;
    std::vector<uint32_t> indices_;

    std::unique_ptr<Vertex> line_;
    std::vector<std::vector<float>> lineVertices_;
    std::vector<std::vector<uint32_t>> lineIndices_;
    std::vector<std::unique_ptr<myVK::Buffer>> lineVertexBuffers_;
    std::vector<std::unique_ptr<myVK::Buffer>> lineIndexBuffers_;
    std::vector<std::vector<std::pair<uint32_t, uint32_t>>> lineOffsets_;
    bool LeftButton_ = false;
    bool LeftButtonOnce_ = false;


    float x_;
    float y_;
    bool ok_ = false;
    
    enum Color {
        Write, 
        Red, 
        Green, 
        Blue, 
    };

    Color color_ = Write;

    struct UniformBufferObject {
        alignas(16) glm::mat4 model_;
        alignas(16) glm::mat4 view_;
        alignas(16) glm::mat4 proj_;
    };
};