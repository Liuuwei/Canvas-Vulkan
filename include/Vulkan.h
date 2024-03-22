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
#include "ktx.h"
#include "vulkan/vulkan_core.h"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <utility>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

#include "Tools.h"
#include "DescriptorSetLayout.h"
#include "RenderPass.h"
#include "Pipeline.h"
#include "Swapchain.h"
#include "FrameBuffer.h"
#include "CommandPool.h"
#include "CommandBuffer.h"
#include "DescriptorPool.h"
#include "Sampler.h"
#include "Camera.h"
#include "Timer.h"
#include "Line.h"
#include "Font.h"

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
    void draw();
    void loadAssets();
    void updateDrawAssets();
    void recreateSwapChain();
    void loadChars();

private:
    bool checkValidationLayerSupport() ;
    bool checkDeviceExtensionsSupport(VkPhysicalDevice physicalDevice);
    bool deviceSuitable(VkPhysicalDevice);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
    VkSampleCountFlagBits getMaxUsableSampleCount();
    VkFormat findDepthFormat();
    VkFormat findSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags features);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue queue);
    void fillColor(uint32_t index);

    void createBrushPipeline();
    void createCanvasPipeline();
    void createTextPipeline();

    void createBrushDescriptorPool();
    void createTextDescriptorPool();
    void createCanvasDescriptorPool();

    void createBrushDescriptorSetLayout();
    void createTextDescriptorSetLayout();
    void createCanvasDescriptorSetLayout();

    void createBrushDescriptorSet();
    void createTextDescriptorSet();
    void createCanvasDescriptorSet();

    void changePoint();
    bool validPoint(int x, int y);
    
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

    std::unique_ptr<Sampler> brushSampler_;
    std::unique_ptr<Sampler> canvasSampler_;

    std::unique_ptr<DescriptorPool> brushDescriptorPool_;
    std::unique_ptr<DescriptorSetLayout> brushDescriptorSetLayout_;
    VkDescriptorSet brushDescriptorSets_;

    std::unique_ptr<DescriptorPool> canvasDescriptorPool_;
    std::unique_ptr<DescriptorSetLayout> canvasDescriptorSetLayout_;
    VkDescriptorSet canvasDescriptorSets_;

    std::unique_ptr<PipelineLayout> brushPipelineLayout_;
    std::unique_ptr<Pipeline> brushPipeline_;

    std::unique_ptr<PipelineLayout> canvasPipelineLayout_;
    std::unique_ptr<Pipeline> canvasPipeline_;

    std::unique_ptr<Image> colorImage_;

    std::unique_ptr<Image> depthImage_;

    std::vector<std::unique_ptr<FrameBuffer>> frameBuffers_;

    std::unique_ptr<CommandPool> commandPool_;
    std::unique_ptr<CommandBuffer> commandBuffers_;

    std::unique_ptr<Fence> inFlightFences_;
    std::unique_ptr<Semaphore> imageAvaiableSemaphores_;
    std::unique_ptr<Semaphore> renderFinishSemaphores_;

    const std::string skyBoxPath_ = "../textures/skybox.ktx";
    ktxTexture* skyBoxTexture_;
    std::unique_ptr<Image> skyBoxImage_;

    const std::string canvasTexturePath_ = "../textures/canvas-texture.jpg";
    std::unique_ptr<Image> canvasImage_;

    Tools::QueueFamilyIndices queueFamilies_;
    VkSampleCountFlagBits msaaSamples_ = VK_SAMPLE_COUNT_1_BIT;

    const std::vector<const char*> validationLayers_ = {"VK_LAYER_KHRONOS_validation"};
    const std::vector<const char*> deviceExtensions_ = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    std::unique_ptr<Buffer> uniformBuffers_;

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

    std::unique_ptr<Plane> canvas_;
    std::vector<Plane::Point> canvasVertices_;
    std::vector<uint32_t> canvasIndices_;
    std::unique_ptr<Buffer> canvasVertexBuffer_;
    std::unique_ptr<Buffer> canvasIndexBuffer_;
    std::unique_ptr<Buffer> canvasUniformBuffer_;

    std::unique_ptr<Line> line_;
    std::vector<Line::Point> lineVertices_;
    std::vector<uint32_t> lineIndices_;
    std::unique_ptr<Buffer> lineVertexBuffers_;
    std::unique_ptr<Buffer> lineIndexBuffers_;
    std::vector<uint32_t> lineVertexMaps_;
    float lineWidth_ = 1.0f;
    bool LeftButton_ = false;
    bool LeftButtonOnce_ = false;
    bool LeftButtonOnceIn_ = false;
    bool prevCursorHandled_ = false;

    glm::vec2 prevCursor_{};
    glm::vec2 currCursor_{};
    glm::vec2 prevCursorRelative_{};
    glm::vec2 currCursorRelative_{};
    bool ok_ = false;
    
    int times_ = 0;

    std::unique_ptr<Font> font_;
    const std::string fontPath_ = "../fonts/jbMono.ttf";
    std::unordered_map<char, Font::Character> dictionary_;
    std::unique_ptr<PipelineLayout> fontPipelineLayout_;
    std::unique_ptr<Pipeline> fontPipeline_;
    std::unique_ptr<DescriptorPool> fontDescriptorPool_;
    std::unique_ptr<DescriptorSetLayout> fontDescriptorSetLayout_;
    VkDescriptorSet fontDescriptorSet_;
    std::vector<Font::Point> fontVertices_;
    std::vector<uint32_t> fontIndices_;
    std::unique_ptr<Buffer> fontVertexBuffer_;
    std::unique_ptr<Buffer> fontIndexBuffer_;

    int inputText_ = 0;
    std::string text_;

    enum Color {
        Write, 
        Red, 
        Green, 
        Blue, 
        Black, 
    };

    const glm::vec3 write3_ = glm::vec3(1.0f, 1.0f, 1.0f);
    const glm::vec3 red3_ = glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::vec3 green3_ = glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 blue3_ = glm::vec3(0.0f, 0.0f, 1.0f);
    const glm::vec3 black3_ = glm::vec3(0.0f, 0.0f, 0.0f);


    Color color_ = Write;

    struct UniformBufferObject {
        alignas(16) glm::mat4 model_;
        alignas(16) glm::mat4 view_;
        alignas(16) glm::mat4 proj_;
    };
};

template<>
struct std::hash<std::pair<int, int>> {
    size_t operator() (const std::pair<int, int>& pair) const {
        return std::hash<int>()(pair.first) ^ std::hash<int>()(pair.second);
    }
};