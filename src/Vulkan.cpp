#include "Vulkan.h"
#include "Buffer.h"
#include "CommandBuffer.h"
#include "CommandPool.h"
#include "DescriptorPool.h"
#include "DescriptorSetLayout.h"
#include "FrameBuffer.h"
#include "GLFW/glfw3.h"
#include "Image.h"
#include "Pipeline.h"
#include "PipelineLayout.h"
#include "Swapchain.h"
#include "Tools.h"
#include "vulkan/vulkan_core.h"
#include <_mingw_stat64.h>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <iostream>
#include <sys/stat.h>
#include <unordered_set>
#include <ktx.h>
#include <algorithm>
#include <utility>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define GLM_FORCE_RADIANS
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#include "ShaderModule.h"
#include "Plane.h"
#include "Line.h"

Vulkan::Vulkan(const std::string& title, uint32_t width, uint32_t height) : width_(width), height_(height), title_(title) {
    camera_ = std::make_shared<Camera>();
    initWindow();
    initVulkan();
}

void Vulkan::run() {
    while (!glfwWindowShouldClose(windows_)) {
        glfwPollEvents();
        draw();
    }

    vkDeviceWaitIdle(device_);
}

void Vulkan::initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

    windows_ = glfwCreateWindow(width_, height_, title_.c_str(), nullptr, nullptr);
    // glfwSetInputMode(windows_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetWindowUserPointer(windows_, this);
    glfwSetKeyCallback(windows_, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        auto vulkan = reinterpret_cast<Vulkan*>(glfwGetWindowUserPointer(window));

        if (action == GLFW_PRESS) {
            switch (key) {
            case GLFW_KEY_0:
                vulkan->color_ = Color::Write;
                break;
            case GLFW_KEY_1:
                vulkan->color_ = Color::Red;
                break;
            case GLFW_KEY_2:
                vulkan->color_ = Color::Green;
                break;
            case GLFW_KEY_3:
                vulkan->color_ = Color::Blue;
                break;
            case GLFW_KEY_4:
                vulkan->color_ = Color::Black;
                break;
            case GLFW_KEY_UP:
                vulkan->lineWidth_++;
                break; 
            case GLFW_KEY_DOWN:
                vulkan->lineWidth_--;
                vulkan->lineWidth_ = std::max(1.0f, vulkan->lineWidth_);
                break; 
            }
        }

        auto camera = vulkan->camera_;
        camera->onKey(window, key, scancode, action, mods);
    });
    glfwSetCursorPosCallback(windows_, [](GLFWwindow* window, double xpos, double ypos) {
        auto vulkan = reinterpret_cast<Vulkan*>(glfwGetWindowUserPointer(window));

        if (vulkan->LeftButton_) {
            vulkan->ok_ = true;
            if (vulkan->prevCursor_.x == -1 && vulkan->prevCursor_.y == -1) {
                vulkan->prevCursor_.x = xpos;
                vulkan->prevCursor_.y = ypos;
                vulkan->currCursor_.x = xpos;
                vulkan->currCursor_.y = ypos;
            } else {
                if (vulkan->prevCursorHandled_) {
                    vulkan->prevCursor_.x = vulkan->currCursor_.x;
                    vulkan->prevCursor_.y = vulkan->currCursor_.y;
                    vulkan->prevCursorHandled_ = false;
                }
                vulkan->currCursor_.x = xpos;
                vulkan->currCursor_.y = ypos;
            }
            vulkan->prevCursorRelative_.x = vulkan->prevCursor_.x - vulkan->swapChain_->width() / 2.0f;
            vulkan->prevCursorRelative_.y = -(vulkan->prevCursor_.y - vulkan->swapChain_->height() / 2.0f);
            vulkan->currCursorRelative_.x = vulkan->currCursor_.x - vulkan->swapChain_->width() / 2.0f;
            vulkan->currCursorRelative_.y = -(vulkan->currCursor_.y - vulkan->swapChain_->height() / 2.0f);
        }

        auto& p = vulkan->prevCursor_;
        auto& c = vulkan->currCursor_;
        auto& rp = vulkan->prevCursorRelative_;
        auto& rc = vulkan->currCursorRelative_;

        auto camera = vulkan->camera_;
        camera->onMouseMovement(window, xpos, ypos);
    });
    glfwSetMouseButtonCallback(windows_, [](GLFWwindow* window, int button, int action, int mods) {
        auto vulkan = reinterpret_cast<Vulkan*>(glfwGetWindowUserPointer(window));
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                glfwGetCursorPos(window, reinterpret_cast<double*>(&vulkan->currCursor_.x), reinterpret_cast<double*>(&vulkan->currCursor_.y));
                vulkan->LeftButton_ = true;
                vulkan->LeftButtonOnce_ = true;
                vulkan->prevCursorHandled_ = true;
            } else {
                vulkan->ok_ = false;
                vulkan->LeftButton_ = false;
                vulkan->LeftButtonOnce_ = false;
                vulkan->prevCursor_.x = -1;
                vulkan->prevCursor_.y = -1;
            }
        }
    });
}

void Vulkan::initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicDevice();
    createSwapChain();
    createRenderPass();
    createCommandPool();
    createCommandBuffers();
    createUniformBuffers();
    loadAssets();
    createSamplers();
    createDescriptorPool();
    createDescriptorSetLayout();
    createDescriptorSet();
    createVertex();
    createGraphicsPipelines();
    createColorResource();
    createDepthResource();
    createFrameBuffer();
    createVertexBuffer();
    createIndexBuffer();
    createSyncObjects();
}

void Vulkan::createInstance() {
    if (!checkValidationLayerSupport()) {
        std::runtime_error("Validation layers requested, but no avaliable!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.apiVersion = VK_API_VERSION_1_3;

    
    auto extensions = Tools::getRequiredExtensions();
    VkInstanceCreateInfo creatInfo{};
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    Tools::populateDebugMessengerCreateInfo(debugCreateInfo, debugCallback);
    creatInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    creatInfo.pApplicationInfo = &appInfo;
    creatInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers_.size());
    creatInfo.ppEnabledLayerNames = validationLayers_.data();
    creatInfo.enabledExtensionCount = extensions.size();
    creatInfo.ppEnabledExtensionNames = extensions.data();
    creatInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;

    if (vkCreateInstance(&creatInfo, nullptr, &instance_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
}

void Vulkan::setupDebugMessenger() {
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    Tools::populateDebugMessengerCreateInfo(createInfo, debugCallback);

    if (Tools::createDebugUtilsMessengerEXT(instance_, &createInfo, nullptr, &debugMessenger_) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

void Vulkan::createSurface() {
    if (glfwCreateWindowSurface(instance_, windows_, nullptr, &surface_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

void Vulkan::pickPhysicalDevice() {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance_, &count, nullptr);
    if (count == 0) {
        throw std::runtime_error("faield to find GPU with Vulkan support!");
    }
    std::vector<VkPhysicalDevice> physicalDevices(count);
    vkEnumeratePhysicalDevices(instance_, &count, physicalDevices.data());

    for (const auto& device : physicalDevices) {
        VkPhysicalDeviceProperties pro;
        vkGetPhysicalDeviceProperties(device, &pro);
        if (pro.deviceName[0] == 'A') {
            continue;
        }
        if (deviceSuitable(device)) {
            physicalDevice_ = device;
            msaaSamples_ = getMaxUsableSampleCount();
            break;
        }
    }

    if (physicalDevice_ == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable physical device!");
    }
}

void Vulkan::createLogicDevice() {
    std::vector<VkDeviceQueueCreateInfo> queueInfos;
    float queuePriority = 1.0f;
    for (auto queueFamily : queueFamilies_.sets()) {
        VkDeviceQueueCreateInfo queueInfo{};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = queueFamily;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &queuePriority;

        queueInfos.push_back(queueInfo);
    }

    VkDeviceCreateInfo deviceInfo{};
    VkPhysicalDeviceFeatures features{};
    features.wideLines = VK_TRUE;
    features.sparseBinding = VK_TRUE;
    features.samplerAnisotropy = VK_TRUE;
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.queueCreateInfoCount = queueInfos.size();
    deviceInfo.pQueueCreateInfos = queueInfos.data();
    deviceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers_.size());
    deviceInfo.ppEnabledLayerNames = validationLayers_.data();
    deviceInfo.enabledExtensionCount = deviceExtensions_.size();
    deviceInfo.ppEnabledExtensionNames = deviceExtensions_.data();
    deviceInfo.pEnabledFeatures = &features;

    if (vkCreateDevice(physicalDevice_, &deviceInfo, nullptr, &device_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logic device!");
    }

    vkGetDeviceQueue(device_, queueFamilies_.graphics.value(), 0, &graphicsQueue_);
    vkGetDeviceQueue(device_, queueFamilies_.present.value(), 0, &presentQueue_);
    vkGetDeviceQueue(device_, queueFamilies_.transfer.value(), 0, &transferQueue_);
}

void Vulkan::createSwapChain() {
    auto swapChainSupport = Tools::SwapChainSupportDetail::querySwapChainSupport(physicalDevice_, surface_);
    
    auto surfaceFormat = swapChainSupport.chooseSurfaceFormat();
    auto presentMode = swapChainSupport.choosePresentMode();
    auto extent = swapChainSupport.chooseSwapExtent(windows_);
    
    uint32_t imageCount = 0;
    imageCount = swapChainSupport.capabilities_.minImageCount + 1;
    if (swapChainSupport.capabilities_.maxImageCount > 0) {
        imageCount = std::min(imageCount, swapChainSupport.capabilities_.maxImageCount);
    }

    swapChain_ = std::make_unique<SwapChain>(device_);
    swapChain_->surface_ = surface_;
    swapChain_->minImageCount_ = imageCount;
    swapChain_->imageFormat_ = surfaceFormat.format;
    swapChain_->imageColorSpace_ = surfaceFormat.colorSpace;
    swapChain_->imageExtent_ = extent;
    swapChain_->imageArrayLayers_ = 1;
    swapChain_->imageUsage_ = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapChain_->imageSharingMode_ = queueFamilies_.multiple() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    swapChain_->queueFamilyIndexCount_ = queueFamilies_.sets().size();
    swapChain_->pQueueFamilyIndices_ = queueFamilies_.sets().data();
    swapChain_->preTransform_ = swapChainSupport.capabilities_.currentTransform;
    swapChain_->compositeAlpha_ = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChain_->presentMode_ = presentMode;
    swapChain_->clipped_ = VK_TRUE;
    swapChain_->oldSwapchain_ = VK_NULL_HANDLE;
    swapChain_->init();
}

void Vulkan::createRenderPass() {
    VkAttachmentDescription colorAttachment{}, colorAttachmentResolve{}, depthAttachment{};

    colorAttachment.format = swapChain_->format();
    colorAttachment.samples = msaaSamples_;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    colorAttachmentResolve.format = swapChain_->format();
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = msaaSamples_;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{}, colorAttachmentResolveRef{}, depthAttachmentRef{};
    
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    colorAttachmentResolveRef.attachment = 1;
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    depthAttachmentRef.attachment = 2;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pResolveAttachments = &colorAttachmentResolveRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency subpassDependency{};
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass = 0;
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    subpassDependency.srcAccessMask =  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 3> attachment{colorAttachment, colorAttachmentResolve, depthAttachment};

    renderPass_ = std::make_unique<RenderPass>(device_);
    renderPass_->subpassCount_ = 1;
    renderPass_->pSubpasses_ = &subpass;
    renderPass_->dependencyCount_ = 1;
    renderPass_->pDependencies_ = &subpassDependency;
    renderPass_->attachmentCount_ = static_cast<uint32_t>(attachment.size());
    renderPass_->pAttachments_ = attachment.data();
    renderPass_->init();
}

void Vulkan::createUniformBuffers() {
    uniformBuffers_.resize(MAX_FRAMES_IN_FLIGHT);
    auto size = sizeof(UniformBufferObject);

    for (size_t i = 0; i < uniformBuffers_.size(); i++) {
        uniformBuffers_[i] = std::make_unique<Buffer>(physicalDevice_, device_);
        uniformBuffers_[i]->size_ = size;
        uniformBuffers_[i]->usage_ = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        uniformBuffers_[i]->queueFamilyIndexCount_ = static_cast<uint32_t>(queueFamilies_.sets().size());
        uniformBuffers_[i]->pQueueFamilyIndices_ = queueFamilies_.sets().data();
        uniformBuffers_[i]->memoryProperties_ = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        uniformBuffers_[i]->sharingMode_ = VK_SHARING_MODE_EXCLUSIVE;
        uniformBuffers_[i]->init();

        uniformBuffers_[i]->map(size);
    }

    canvasUniformBuffer_ = std::make_unique<Buffer>(physicalDevice_, device_);
    canvasUniformBuffer_->size_ = size;
    canvasUniformBuffer_->usage_ = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    canvasUniformBuffer_->queueFamilyIndexCount_ = static_cast<uint32_t>(queueFamilies_.sets().size());
    canvasUniformBuffer_->pQueueFamilyIndices_ = queueFamilies_.sets().data();
    canvasUniformBuffer_->memoryProperties_ = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    canvasUniformBuffer_->sharingMode_ = VK_SHARING_MODE_EXCLUSIVE;
    canvasUniformBuffer_->init();
    auto data = canvasUniformBuffer_->map(sizeof(UniformBufferObject));
    UniformBufferObject ubo{};
    ubo.proj_ = glm::ortho(-static_cast<float>(swapChain_->width() / 2.0f), static_cast<float>(swapChain_->width() / 2.0f), -static_cast<float>(swapChain_->height() / 2.0), static_cast<float>(swapChain_->height() / 2.0f));
    memcpy(data, &ubo, sizeof(UniformBufferObject));
    canvasUniformBuffer_->unMap();
}

void Vulkan::createSamplers() {
    brushSampler_ = std::make_unique<Sampler>(device_);
    brushSampler_->addressModeU_ = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    brushSampler_->addressModeV_ = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    brushSampler_->addressModeW_ = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    brushSampler_->minLod_ = 0.0f;
    brushSampler_->maxLod_ = 1.0f;
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physicalDevice_, &properties);
    brushSampler_->anisotropyEnable_ = VK_TRUE;
    brushSampler_->maxAnisotropy_ = properties.limits.maxSamplerAnisotropy;
    brushSampler_->init();

    canvasSampler_ = std::make_unique<Sampler>(device_);
    canvasSampler_->addressModeU_ = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    canvasSampler_->addressModeV_ = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    canvasSampler_->addressModeW_ = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    canvasSampler_->minLod_ = 0.0f;
    canvasSampler_->maxLod_ = 1.0f;
    canvasSampler_->anisotropyEnable_ = VK_TRUE;
    canvasSampler_->maxAnisotropy_ = properties.limits.maxSamplerAnisotropy;
    canvasSampler_->init();
}

void Vulkan::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> poolSizes;
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    brushDescriptorPool_ = std::make_unique<DescriptorPool>(device_);
    brushDescriptorPool_->poolSizeCount_ = static_cast<uint32_t>(poolSizes.size());
    brushDescriptorPool_->pPoolSizes_ = poolSizes.data();
    brushDescriptorPool_->maxSets_ = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    brushDescriptorPool_->init();

    canvasDescriptorPool_ = std::make_unique<DescriptorPool>(device_);
    canvasDescriptorPool_->poolSizeCount_ = static_cast<uint32_t>(poolSizes.size());
    canvasDescriptorPool_->pPoolSizes_ = poolSizes.data();
    canvasDescriptorPool_->maxSets_ = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    canvasDescriptorPool_->init();
}

void Vulkan::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboBinding{}, samplerBinding{};
    uboBinding.binding = 0;
    uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    samplerBinding.binding = 1;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.descriptorCount = 1;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboBinding, samplerBinding};

    brushDescriptorSetLayout_ = std::make_unique<DescriptorSetLayout>(device_);
    brushDescriptorSetLayout_->bindingCount_ = static_cast<uint32_t>(bindings.size());
    brushDescriptorSetLayout_->pBindings_ = bindings.data();
    brushDescriptorSetLayout_->init();

    canvasDescriptorSetLayout_ = std::make_unique<DescriptorSetLayout>(device_);
    canvasDescriptorSetLayout_->bindingCount_ = static_cast<uint32_t>(bindings.size());
    canvasDescriptorSetLayout_->pBindings_ = bindings.data();
    canvasDescriptorSetLayout_->init();
}

void Vulkan::createDescriptorSet() {
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts(MAX_FRAMES_IN_FLIGHT, brushDescriptorSetLayout_->descriptorSetLayout());

    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = brushDescriptorPool_->descriptorPool();
    allocateInfo.descriptorSetCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    allocateInfo.pSetLayouts = descriptorSetLayouts.data();

    brushDescriptorSets_.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device_, &allocateInfo, brushDescriptorSets_.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < brushDescriptorSets_.size(); i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers_[i]->buffer();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo samplerInfo{};
        samplerInfo.imageView = canvasImage_->view();
        samplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        samplerInfo.sampler = brushSampler_->sampler();

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = brushDescriptorSets_[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = brushDescriptorSets_[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].pImageInfo = &samplerInfo;

        vkUpdateDescriptorSets(device_, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

    descriptorSetLayouts.resize(1, canvasDescriptorSetLayout_->descriptorSetLayout());
    allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = canvasDescriptorPool_->descriptorPool();
    allocateInfo.descriptorSetCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    allocateInfo.pSetLayouts = descriptorSetLayouts.data();
    if (vkAllocateDescriptorSets(device_, &allocateInfo, &canvasDescriptorSets_) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor set");
    }
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = canvasUniformBuffer_->buffer();
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageView = canvasImage_->view();
    imageInfo.sampler = canvasSampler_->sampler();
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = canvasDescriptorSets_;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = canvasDescriptorSets_;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device_, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Vulkan::createVertex() {
    canvas_ = std::make_unique<Plane>();
    auto t = canvas_->vertices(swapChain_->width(), swapChain_->height());
    canvasVertices_ = t.first;
    canvasIndices_ = t.second;

    line_ = std::make_unique<Line>();
    lineVertices_.resize(MAX_FRAMES_IN_FLIGHT);
    lineIndices_.resize(MAX_FRAMES_IN_FLIGHT);  
    lineVertexBuffers_.resize(MAX_FRAMES_IN_FLIGHT);
    lineIndexBuffers_.resize(MAX_FRAMES_IN_FLIGHT);  
    lineVerticesModify_.resize(MAX_FRAMES_IN_FLIGHT, 0);
    lineVertexMaps_.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        auto t = line_->initVertices(swapChain_->width(), swapChain_->height());
        lineVertices_[i] = t.first;
        lineIndices_[i] = t.second;

        for (size_t j = 0; j < lineVertices_[i].size(); j++) {
            auto key = std::make_pair(lineVertices_[i][j].position_.x, lineVertices_[i][j].position_.y);
            lineVertexMaps_[i][key] = j;
        }
    }

    lineOffsets_.resize(MAX_FRAMES_IN_FLIGHT);
}

void Vulkan::createBrushPipeline() {
    auto vertBrush = ShaderModule(device_, "../shaders/spv/VertBrush.spv"); 
    auto fragBrush = ShaderModule(device_, "../shaders/spv/FragBrush.spv");

    VkPipelineShaderStageCreateInfo vertexStageInfo{}, fragmentStageInfo{};
    vertexStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexStageInfo.module = vertBrush.shader();
    vertexStageInfo.pName = "main";

    fragmentStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentStageInfo.module = fragBrush.shader();
    fragmentStageInfo.pName = "main";

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages{vertexStageInfo, fragmentStageInfo};
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    auto bindingDescription = line_->bindingDescription(0);
    auto attributeDescription = line_->attributeDescription(0);

    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescription.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology = line_->topology();

    VkPipelineViewportStateCreateInfo viewportInfo{};
    viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.viewportCount = 1;
    viewportInfo.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizaInfo{};
    rasterizaInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizaInfo.polygonMode = VK_POLYGON_MODE_FILL;
    // rasterizaInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizaInfo.cullMode = VK_CULL_MODE_NONE;
    rasterizaInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizaInfo.lineWidth = 5.0f;

    VkPipelineMultisampleStateCreateInfo multipleInfo{};
    multipleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multipleInfo.rasterizationSamples = msaaSamples_;
    multipleInfo.minSampleShading = 1.0f;
    
    VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
    depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilInfo.depthTestEnable = VK_TRUE;
    depthStencilInfo.depthWriteEnable = VK_TRUE;
    depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilInfo.minDepthBounds = 0.0f;
    depthStencilInfo.maxDepthBounds = 1.0f;

    VkPipelineColorBlendAttachmentState colorBlendAttachmentInfo{};
    colorBlendAttachmentInfo.blendEnable = VK_FALSE;
    colorBlendAttachmentInfo.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachmentInfo.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachmentInfo.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachmentInfo.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachmentInfo.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachmentInfo.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachmentInfo.alphaBlendOp = VK_BLEND_OP_ADD;
    
    VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
    colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendInfo.attachmentCount = 1;
    colorBlendInfo.pAttachments = &colorBlendAttachmentInfo;

    std::array<VkDynamicState, 3> dynamics{
        VK_DYNAMIC_STATE_VIEWPORT, 
        VK_DYNAMIC_STATE_SCISSOR, 
        VK_DYNAMIC_STATE_LINE_WIDTH, 
    };
    VkPipelineDynamicStateCreateInfo dynamicInfo{};
    dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicInfo.dynamicStateCount = static_cast<uint32_t>(dynamics.size());
    dynamicInfo.pDynamicStates = dynamics.data();

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts = {brushDescriptorSetLayout_->descriptorSetLayout()};
    brushPipelineLayout_ = std::make_unique<PipelineLayout>(device_);
    brushPipelineLayout_->setLayoutCount_ = static_cast<uint32_t>(descriptorSetLayouts.size());
    brushPipelineLayout_->pSetLayouts_ = descriptorSetLayouts.data();
    brushPipelineLayout_->init();

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    brushPipeline_ = std::make_unique<Pipeline>(device_);
    brushPipeline_->stageCount_ = shaderStages.size();
    brushPipeline_->pStages_ = shaderStages.data();
    brushPipeline_->pVertexInputState_ = &vertexInputInfo;
    brushPipeline_->pInputAssemblyState_ = &inputAssemblyInfo;
    brushPipeline_->pViewportState_ = &viewportInfo;
    brushPipeline_->pRasterizationState_ = &rasterizaInfo;;
    brushPipeline_->pMultisampleState_ = &multipleInfo;
    brushPipeline_->pDepthStencilState_ = &depthStencilInfo;
    brushPipeline_->pColorBlendState_ = &colorBlendInfo;
    brushPipeline_->pDynamicState_ = &dynamicInfo;
    brushPipeline_->layout_ = brushPipelineLayout_->pipelineLayout();
    brushPipeline_->renderPass_ = renderPass_->renderPass();
    brushPipeline_->init();
}

void Vulkan::createCanvasPipeline() {
    auto vertCanvas = ShaderModule(device_, "../shaders/spv/VertCanvas.spv");
    auto fragCanvas = ShaderModule(device_, "../shaders/spv/FragCanvas.spv");

    VkPipelineShaderStageCreateInfo vertexStageInfo{}, fragmentStageInfo{};
    vertexStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexStageInfo.module = vertCanvas.shader();
    vertexStageInfo.pName = "main";

    fragmentStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentStageInfo.module = fragCanvas.shader();
    fragmentStageInfo.pName = "main";

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages{vertexStageInfo, fragmentStageInfo};
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    auto bindingDescription = canvas_->bindingDescription(0);
    auto attributeDescription = canvas_->attributeDescription(0);

    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescription.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology = canvas_->topology();

    VkPipelineViewportStateCreateInfo viewportInfo{};
    viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.viewportCount = 1;
    viewportInfo.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizaInfo{};
    rasterizaInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizaInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizaInfo.cullMode = VK_CULL_MODE_NONE;
    rasterizaInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizaInfo.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multipleInfo{};
    multipleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multipleInfo.rasterizationSamples = msaaSamples_;
    multipleInfo.minSampleShading = 1.0f;
    
    VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
    depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilInfo.depthTestEnable = VK_TRUE;
    depthStencilInfo.depthWriteEnable = VK_TRUE;
    depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilInfo.minDepthBounds = 0.0f;
    depthStencilInfo.maxDepthBounds = 1.0f;

    VkPipelineColorBlendAttachmentState colorBlendAttachmentInfo{};
    colorBlendAttachmentInfo.blendEnable = VK_TRUE;
    colorBlendAttachmentInfo.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachmentInfo.srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    colorBlendAttachmentInfo.dstColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
    colorBlendAttachmentInfo.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachmentInfo.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachmentInfo.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachmentInfo.alphaBlendOp = VK_BLEND_OP_ADD;
    
    VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
    colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendInfo.attachmentCount = 1;
    colorBlendInfo.pAttachments = &colorBlendAttachmentInfo;

    std::array<VkDynamicState, 2> dynamics{
        VK_DYNAMIC_STATE_VIEWPORT, 
        VK_DYNAMIC_STATE_SCISSOR, 
    };
    VkPipelineDynamicStateCreateInfo dynamicInfo{};
    dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicInfo.dynamicStateCount = static_cast<uint32_t>(dynamics.size());
    dynamicInfo.pDynamicStates = dynamics.data();

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts = {canvasDescriptorSetLayout_->descriptorSetLayout()};
    canvasPipelineLayout_ = std::make_unique<PipelineLayout>(device_);
    canvasPipelineLayout_->setLayoutCount_ = static_cast<uint32_t>(descriptorSetLayouts.size());
    canvasPipelineLayout_->pSetLayouts_ = descriptorSetLayouts.data();
    canvasPipelineLayout_->init();

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    canvasPipeline_ = std::make_unique<Pipeline>(device_);
    canvasPipeline_->stageCount_ = shaderStages.size();
    canvasPipeline_->pStages_ = shaderStages.data();
    canvasPipeline_->pVertexInputState_ = &vertexInputInfo;
    canvasPipeline_->pInputAssemblyState_ = &inputAssemblyInfo;
    canvasPipeline_->pViewportState_ = &viewportInfo;
    canvasPipeline_->pRasterizationState_ = &rasterizaInfo;;
    canvasPipeline_->pMultisampleState_ = &multipleInfo;
    canvasPipeline_->pDepthStencilState_ = &depthStencilInfo;
    canvasPipeline_->pColorBlendState_ = &colorBlendInfo;
    canvasPipeline_->pDynamicState_ = &dynamicInfo;
    canvasPipeline_->layout_ = canvasPipelineLayout_->pipelineLayout();
    canvasPipeline_->renderPass_ = renderPass_->renderPass();
    canvasPipeline_->init();
}

void Vulkan::createGraphicsPipelines() {
    createCanvasPipeline();
    createBrushPipeline();
}

void Vulkan::createColorResource() {
    colorImage_ = std::make_unique<Image>(physicalDevice_, device_);
    colorImage_->imageType_ = VK_IMAGE_TYPE_2D;
    colorImage_->format_ = swapChain_->format();
    colorImage_->extent_ = {swapChain_->extent().width, swapChain_->extent().height, 1};
    colorImage_->mipLevles_ = 1;
    colorImage_->arrayLayers_ = 1;
    colorImage_->samples_ = msaaSamples_;
    colorImage_->tiling_ = VK_IMAGE_TILING_OPTIMAL;
    colorImage_->usage_ = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    colorImage_->sharingMode_ = queueFamilies_.multiple() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    colorImage_->queueFamilyIndexCount_ = static_cast<uint32_t>(queueFamilies_.sets().size());
    colorImage_->pQueueFamilyIndices_ = queueFamilies_.sets().data();
    colorImage_->viewType_ = VK_IMAGE_VIEW_TYPE_2D;
    colorImage_->subresourcesRange_ = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    colorImage_->memoryProperties_ = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    colorImage_->init();
}

void Vulkan::createDepthResource() {
    depthImage_ = std::make_unique<Image>(physicalDevice_, device_);
    depthImage_->imageType_ = VK_IMAGE_TYPE_2D;
    depthImage_->format_ = findDepthFormat();
    depthImage_->mipLevles_ = 1;
    depthImage_->arrayLayers_ = 1;
    depthImage_->samples_ = msaaSamples_;
    depthImage_->extent_ = {swapChain_->extent().width, swapChain_->extent().height, 1};
    depthImage_->tiling_ = VK_IMAGE_TILING_OPTIMAL;
    depthImage_->usage_ = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depthImage_->sharingMode_ = queueFamilies_.multiple() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    depthImage_->queueFamilyIndexCount_ = static_cast<uint32_t>(queueFamilies_.sets().size());
    depthImage_->pQueueFamilyIndices_ = queueFamilies_.sets().data();
    depthImage_->viewType_ = VK_IMAGE_VIEW_TYPE_2D;
    depthImage_->subresourcesRange_ = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
    depthImage_->memoryProperties_ = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    depthImage_->init();
}

void Vulkan::createFrameBuffer() {
    frameBuffers_.resize(swapChain_->size());
    
    for (size_t i = 0; i < frameBuffers_.size(); i++) {
        std::array<VkImageView, 3> attachment = {
            colorImage_->view(), 
            swapChain_->imageView(i), 
            depthImage_->view(),   
        };

        frameBuffers_[i] = std::make_unique<FrameBuffer>(device_);
        frameBuffers_[i]->renderPass_ = renderPass_->renderPass();
        frameBuffers_[i]->width_ = swapChain_->width();
        frameBuffers_[i]->height_ = swapChain_->height();
        frameBuffers_[i]->layers_ = 1;
        frameBuffers_[i]->attachmentCount_ = static_cast<uint32_t>(attachment.size());
        frameBuffers_[i]->pAttachments_ = attachment.data();
        frameBuffers_[i]->init();
    }
}

void Vulkan::createCommandPool() {
    commandPool_ = std::make_unique<CommandPool>(device_);
    commandPool_->flags_ = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPool_->queueFamilyIndex_ = queueFamilies_.graphics.value();
    commandPool_->init();
}

void Vulkan::createCommandBuffers() {
    commandBuffers_.resize(MAX_FRAMES_IN_FLIGHT);
    
    for (size_t i = 0; i < commandBuffers_.size(); i++) {
        commandBuffers_[i] = std::make_unique<CommandBuffer>(device_);
        commandBuffers_[i]->commandPool_ = commandPool_->commanddPool();
        commandBuffers_[i]->level_ = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBuffers_[i]->commandBufferCount_ = 1;
        commandBuffers_[i]->init();
    }
}

void Vulkan::createVertexBuffer() {
    VkDeviceSize size = sizeof(float) * canvasVertices_.size();

    canvasVertexBuffer_ = std::make_unique<Buffer>(physicalDevice_, device_);
    canvasVertexBuffer_->size_ = size;
    canvasVertexBuffer_->usage_ = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    canvasVertexBuffer_->sharingMode_ = queueFamilies_.multiple() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    canvasVertexBuffer_->queueFamilyIndexCount_ = static_cast<uint32_t>(queueFamilies_.sets().size());
    canvasVertexBuffer_->pQueueFamilyIndices_ = queueFamilies_.sets().data();
    canvasVertexBuffer_->memoryProperties_ = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    canvasVertexBuffer_->init();

    std::unique_ptr<Buffer> staginBuffer = std::make_unique<Buffer>(physicalDevice_, device_);
    staginBuffer->size_ = size;
    staginBuffer->usage_ = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    staginBuffer->sharingMode_ = queueFamilies_.multiple() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    staginBuffer->queueFamilyIndexCount_ = static_cast<uint32_t>(queueFamilies_.sets().size());
    staginBuffer->pQueueFamilyIndices_ = queueFamilies_.sets().data();
    staginBuffer->memoryProperties_ = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    staginBuffer->init();

    auto data = staginBuffer->map(size);
    memcpy(data, canvasVertices_.data(), size);
    staginBuffer->unMap();

    copyBuffer(staginBuffer->buffer(), canvasVertexBuffer_->buffer(), size);

    for (size_t i = 0; i < lineVertexBuffers_.size(); i++) {
        VkDeviceSize size = sizeof(Line::Point) * lineVertices_[i].size();

        lineVertexBuffers_[i] = std::make_unique<Buffer>(physicalDevice_, device_);
        if (size == 0) {
            continue;
        }
        lineVertexBuffers_[i]->size_ = size;
        lineVertexBuffers_[i]->usage_ = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        lineVertexBuffers_[i]->sharingMode_ = queueFamilies_.multiple() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
        lineVertexBuffers_[i]->queueFamilyIndexCount_ = static_cast<uint32_t>(queueFamilies_.sets().size());
        lineVertexBuffers_[i]->pQueueFamilyIndices_ = queueFamilies_.sets().data();
        lineVertexBuffers_[i]->memoryProperties_ = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        lineVertexBuffers_[i]->init();

        std::unique_ptr<Buffer> staginBuffer = std::make_unique<Buffer>(physicalDevice_, device_);
        staginBuffer->size_ = size;
        staginBuffer->usage_ = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        staginBuffer->sharingMode_ = queueFamilies_.multiple() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
        staginBuffer->queueFamilyIndexCount_ = static_cast<uint32_t>(queueFamilies_.sets().size());
        staginBuffer->pQueueFamilyIndices_ = queueFamilies_.sets().data();
        staginBuffer->memoryProperties_ = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        staginBuffer->init();

        auto data = staginBuffer->map(size);
        memcpy(data, lineVertices_[i].data(), size);
        staginBuffer->unMap();

        copyBuffer(staginBuffer->buffer(), lineVertexBuffers_[i]->buffer(), size);
    }
}

void Vulkan::createIndexBuffer() {
    VkDeviceSize size = sizeof(canvasIndices_[0]) * canvasIndices_.size();

    canvasIndexBuffer_ = std::make_unique<Buffer>(physicalDevice_, device_);
    canvasIndexBuffer_->size_ = size;
    canvasIndexBuffer_->usage_ = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    canvasIndexBuffer_->sharingMode_ = queueFamilies_.multiple() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    canvasIndexBuffer_->queueFamilyIndexCount_ = static_cast<uint32_t>(queueFamilies_.sets().size());
    canvasIndexBuffer_->pQueueFamilyIndices_ = queueFamilies_.sets().data();
    canvasIndexBuffer_->memoryProperties_ = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    canvasIndexBuffer_->init();

    std::unique_ptr<Buffer> staginBuffer = std::make_unique<Buffer>(physicalDevice_, device_);
    staginBuffer->size_ = size;
    staginBuffer->usage_ = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    staginBuffer->sharingMode_ = queueFamilies_.multiple() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    staginBuffer->queueFamilyIndexCount_ = static_cast<uint32_t>(queueFamilies_.sets().size());
    staginBuffer->pQueueFamilyIndices_ = queueFamilies_.sets().data();
    staginBuffer->memoryProperties_ = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    staginBuffer->init();

    auto data = staginBuffer->map(size);
    memcpy(data, canvasIndices_.data(), size);
    staginBuffer->unMap();

    copyBuffer(staginBuffer->buffer(), canvasIndexBuffer_->buffer(), size);
    
    for (size_t i = 0; i < lineVertexBuffers_.size(); i++) {
        VkDeviceSize size = sizeof(uint32_t) * lineIndices_[i].size();

        lineIndexBuffers_[i] = std::make_unique<Buffer>(physicalDevice_, device_);
        if (size == 0) {
            continue;
        }
        lineIndexBuffers_[i]->size_ = size;
        lineIndexBuffers_[i]->usage_ = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        lineIndexBuffers_[i]->sharingMode_ = queueFamilies_.multiple() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
        lineIndexBuffers_[i]->queueFamilyIndexCount_ = static_cast<uint32_t>(queueFamilies_.sets().size());
        lineIndexBuffers_[i]->pQueueFamilyIndices_ = queueFamilies_.sets().data();
        lineIndexBuffers_[i]->memoryProperties_ = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        lineIndexBuffers_[i]->init();

        std::unique_ptr<Buffer> staginBuffer = std::make_unique<Buffer>(physicalDevice_, device_);
        staginBuffer->size_ = size;
        staginBuffer->usage_ = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        staginBuffer->sharingMode_ = queueFamilies_.multiple() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
        staginBuffer->queueFamilyIndexCount_ = static_cast<uint32_t>(queueFamilies_.sets().size());
        staginBuffer->pQueueFamilyIndices_ = queueFamilies_.sets().data();
        staginBuffer->memoryProperties_ = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        staginBuffer->init();

        auto data = staginBuffer->map(size);
        memcpy(data, lineIndices_[i].data(), size);
        staginBuffer->unMap();

        copyBuffer(staginBuffer->buffer(), lineIndexBuffers_[i]->buffer(), size);
    }
}

void Vulkan::createSyncObjects() {
    inFlightFences_.resize(MAX_FRAMES_IN_FLIGHT);
    imageAvaiableSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        inFlightFences_[i] = std::make_unique<Fence>(device_);
        inFlightFences_[i]->flags_ = VK_FENCE_CREATE_SIGNALED_BIT;
        inFlightFences_[i]->init();
        imageAvaiableSemaphores_[i] = std::make_unique<Semaphore>(device_);
        imageAvaiableSemaphores_[i]->init();
        renderFinishSemaphores_[i] = std::make_unique<Semaphore>(device_);
        renderFinishSemaphores_[i]->init();
    }
}

void Vulkan::recordCommadBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo commandBufferBeginInfo{};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin command buffer!");
    }

    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = renderPass_->renderPass();
    renderPassBeginInfo.framebuffer = frameBuffers_[imageIndex]->frameBuffer();
    renderPassBeginInfo.renderArea.extent = swapChain_->extent();
    std::array<VkClearValue, 3> clearValues;
    clearValues[0].color = {0.0f, 0.0f, 0.0f, 0.0f};
    clearValues[1].color = {0.0f, 0.0f, 0.0f, 0.0f};
    clearValues[2].depthStencil = {1.0f, 0};
    renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassBeginInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = static_cast<float>(swapChain_->extent().height);
        viewport.width = static_cast<float>(swapChain_->extent().width);
        viewport.height = -static_cast<float>(swapChain_->extent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        VkRect2D scissor{};
        scissor.extent = swapChain_->extent();
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        VkDeviceSize offsets[] = {0};

        // Lines
        if (lineVertices_[currentFrame_].size() != 0) {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, brushPipeline_->pipeline());

            vkCmdSetLineWidth(commandBuffer, 20.0f);

            std::array<VkDescriptorSet, 1> descriptorSets{brushDescriptorSets_[currentFrame_]};
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, brushPipelineLayout_->pipelineLayout(), 0, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);

            std::vector<VkBuffer> vertexBuffer = {lineVertexBuffers_[currentFrame_]->buffer()};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffer.data(), offsets);
            vkCmdBindIndexBuffer(commandBuffer, lineIndexBuffers_[currentFrame_]->buffer(), 0, VK_INDEX_TYPE_UINT32);
            
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(lineIndices_[currentFrame_].size()), 1, 0, 0, 0);

            // for (auto p : lineOffsets_[currentFrame_]) {
            //     vkCmdDrawIndexed(commandBuffer, p.second - p.first, 1, p.first, 0, 0);
            // }
        }

        // Canvas
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, canvasPipeline_->pipeline());
        std::vector<VkDescriptorSet> canvasDescriptorSets{canvasDescriptorSets_};
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, canvasPipelineLayout_->pipelineLayout(), 0, static_cast<uint32_t>(canvasDescriptorSets.size()), canvasDescriptorSets.data(), 0, nullptr);
        std::vector<VkBuffer> canvasVertexBuffers ={canvasVertexBuffer_->buffer()};
        vkCmdBindVertexBuffers(commandBuffer, 0, static_cast<uint32_t>(canvasVertexBuffers.size()), canvasVertexBuffers.data(), offsets);
        vkCmdBindIndexBuffer(commandBuffer, canvasIndexBuffer_->buffer(), 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(commandBuffer, canvasIndices_.size(), 1, 0, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to end command buffer!");
    }
}

void Vulkan::draw() {
    timer_.tick();
    camera_->setDeltaTime(timer_.delta());

    vkWaitForFences(device_, 1, inFlightFences_[currentFrame_]->fencePtr(), VK_TRUE, UINT64_MAX);

    uint32_t imageIndex = 0;
    vkAcquireNextImageKHR(device_, swapChain_->swapChain(), UINT64_MAX, imageAvaiableSemaphores_[currentFrame_]->semaphore(), VK_NULL_HANDLE, &imageIndex);

    vkResetFences(device_, 1, inFlightFences_[currentFrame_]->fencePtr());

    vkResetCommandBuffer(commandBuffers_[currentFrame_]->commandBuffer(), 0);

    updateDrawAssets();

    recordCommadBuffer(commandBuffers_[currentFrame_]->commandBuffer(), imageIndex);

    std::array<VkSemaphore, 1> waits = {imageAvaiableSemaphores_[currentFrame_]->semaphore()};
    std::array<VkPipelineStageFlags, 1> waitStages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    std::array<VkCommandBuffer, 1> commandBuffers = {commandBuffers_[currentFrame_]->commandBuffer()};
    std::array<VkSemaphore, 1> signals = {renderFinishSemaphores_[currentFrame_]->semaphore()};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waits.size());
    submitInfo.pWaitSemaphores = waits.data();
    submitInfo.pWaitDstStageMask = waitStages.data();
    submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
    submitInfo.pCommandBuffers = commandBuffers.data();
    submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signals.size());
    submitInfo.pSignalSemaphores = signals.data();

    if (vkQueueSubmit(graphicsQueue_, 1, &submitInfo, inFlightFences_[currentFrame_]->fence()) != VK_SUCCESS) {
        throw std::runtime_error("failed to queue submit!");
    }

    std::array<VkSwapchainKHR, 1> swapChains = {swapChain_->swapChain()};

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = static_cast<uint32_t>(swapChains.size());
    presentInfo.pSwapchains = swapChains.data();
    presentInfo.waitSemaphoreCount = static_cast<uint32_t>(signals.size());
    presentInfo.pWaitSemaphores = signals.data();
    presentInfo.pImageIndices = &imageIndex;

    auto result = vkQueuePresentKHR(presentQueue_, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return ;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("faield to present!");
    }

    currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Vulkan::loadAssets() {
    int texWidth, texHeight, texChannels;
    auto pixel = stbi_load(canvasTexturePath_.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixel) {
        throw std::runtime_error("failed to load canvas texture");
    }

    VkDeviceSize size = texWidth * texHeight * 4;

    canvasImage_ = std::make_unique<Image>(physicalDevice_, device_);
    canvasImage_->imageType_ = VK_IMAGE_TYPE_2D;
    canvasImage_->arrayLayers_ = 1;
    canvasImage_->mipLevles_ = 1;
    canvasImage_->format_ = VK_FORMAT_R8G8B8A8_SRGB;
    canvasImage_->extent_ = {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1};
    canvasImage_->queueFamilyIndexCount_ = static_cast<uint32_t>(queueFamilies_.sets().size());
    canvasImage_->pQueueFamilyIndices_ = queueFamilies_.sets().data();
    canvasImage_->sharingMode_ = queueFamilies_.multiple() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    canvasImage_->tiling_ = VK_IMAGE_TILING_OPTIMAL;
    canvasImage_->usage_ = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    canvasImage_->memoryProperties_ = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    canvasImage_->viewType_ = VK_IMAGE_VIEW_TYPE_2D;
    canvasImage_->samples_ = VK_SAMPLE_COUNT_1_BIT;
    canvasImage_->subresourcesRange_ = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    canvasImage_->init();
    
    std::unique_ptr<Buffer> staginBuffer = std::make_unique<Buffer>(physicalDevice_, device_);
    staginBuffer->size_ = size;
    staginBuffer->queueFamilyIndexCount_ = static_cast<uint32_t>(queueFamilies_.sets().size());
    staginBuffer->pQueueFamilyIndices_ = queueFamilies_.sets().data();
    staginBuffer->sharingMode_ = queueFamilies_.multiple() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    staginBuffer->usage_ = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    staginBuffer->memoryProperties_ = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    staginBuffer->init();
    
    auto data = staginBuffer->map(size);
    memcpy(data, pixel, size);
    staginBuffer->unMap();

    VkImageSubresourceRange range{1, 0, 1, 0, 1};
    VkBufferImageCopy region{};
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1};
    region.imageSubresource = {1, 0, 0, 1};

    auto cmdBuffer = beginSingleTimeCommands();
        Tools::setImageLayout(cmdBuffer, canvasImage_->image(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, range, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

        vkCmdCopyBufferToImage(cmdBuffer, staginBuffer->buffer(), canvasImage_->image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        Tools::setImageLayout(cmdBuffer, canvasImage_->image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, range, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        
    endSingleTimeCommands(cmdBuffer, transferQueue_);
}

void Vulkan::updateDrawAssets() {
    static long long modifyTag = 0;

    UniformBufferObject ubo{};
    ubo.proj_ = glm::ortho(-static_cast<float>(swapChain_->width()) / 2.0f, static_cast<float>(swapChain_->width()) / 2.0f, -static_cast<float>(swapChain_->height()) / 2.0f, static_cast<float>(swapChain_->height()) / 2.0f);

    auto data = uniformBuffers_[currentFrame_]->map(sizeof(ubo));    
    memcpy(data, &ubo, sizeof(ubo));
    {   
        if (ok_ == false || LeftButton_ == false) {
            return ;
        }
        ok_ = false;
        auto& vertices = lineVertices_[currentFrame_];
        auto& indices = lineIndices_[currentFrame_];
        auto& vertexBuffer = lineVertexBuffers_[currentFrame_];
        auto& indexBuffer = lineIndexBuffers_[currentFrame_];
        auto& indexOffset = lineOffsets_[currentFrame_];
        int prevFrame = currentFrame_ - 1;
        if (prevFrame < 0) {
            prevFrame = MAX_FRAMES_IN_FLIGHT - 1;
        }
        if (lineVerticesModify_[prevFrame] > lineVerticesModify_[currentFrame_]) {
            vertices = lineVertices_[prevFrame];            
        }

        changePoint();
        prevCursorHandled_ = true;

        VkDeviceSize size = sizeof(Line::Point) * lineVertices_[currentFrame_].size();

        Buffer staginBuffer(physicalDevice_, device_);
        staginBuffer.size_ = size;
        staginBuffer.usage_ = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        staginBuffer.sharingMode_ = queueFamilies_.multiple() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
        staginBuffer.queueFamilyIndexCount_ = static_cast<uint32_t>(queueFamilies_.sets().size());
        staginBuffer.pQueueFamilyIndices_ = queueFamilies_.sets().data();
        staginBuffer.memoryProperties_ = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        staginBuffer.init();

        auto data = staginBuffer.map(size);
        memcpy(data, lineVertices_[currentFrame_].data(), size);
        staginBuffer.unMap();

        copyBuffer(staginBuffer.buffer(), lineVertexBuffers_[currentFrame_]->buffer(), size);

        lineVerticesModify_[currentFrame_] = modifyTag++;
    }
}   

void Vulkan::recreateSwapChain() {
    int width, height;
    glfwGetWindowSize(windows_, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetWindowSize(windows_, &width, &height);
        glfwPollEvents();
    }

    vkDeviceWaitIdle(device_);

    swapChain_.reset(nullptr);

    createSwapChain();
    createColorResource();
    createDepthResource();
    createFrameBuffer();   

    createVertex();
    createVertexBuffer();
    createIndexBuffer();

    // {
    //     auto t = canvas_->vertices(swapChain_->width(), swapChain_->height());
    //     canvasVertices_ = t.first;
    //     canvasIndices_ = t.second;
    // }

    // {
    //     VkDeviceSize size = sizeof(float) * canvasVertices_.size();

    //     canvasVertexBuffer_ = std::make_unique<Buffer>(physicalDevice_, device_);
    //     canvasVertexBuffer_->size_ = size;
    //     canvasVertexBuffer_->usage_ = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    //     canvasVertexBuffer_->sharingMode_ = queueFamilies_.multiple() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    //     canvasVertexBuffer_->queueFamilyIndexCount_ = static_cast<uint32_t>(queueFamilies_.sets().size());
    //     canvasVertexBuffer_->pQueueFamilyIndices_ = queueFamilies_.sets().data();
    //     canvasVertexBuffer_->memoryProperties_ = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    //     canvasVertexBuffer_->init();

    //     std::unique_ptr<Buffer> staginBuffer = std::make_unique<Buffer>(physicalDevice_, device_);
    //     staginBuffer->size_ = size;
    //     staginBuffer->usage_ = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    //     staginBuffer->sharingMode_ = queueFamilies_.multiple() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    //     staginBuffer->queueFamilyIndexCount_ = static_cast<uint32_t>(queueFamilies_.sets().size());
    //     staginBuffer->pQueueFamilyIndices_ = queueFamilies_.sets().data();
    //     staginBuffer->memoryProperties_ = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    //     staginBuffer->init();

    //     auto data = staginBuffer->map(size);
    //     memcpy(data, canvasVertices_.data(), size);
    //     staginBuffer->unMap();

    //     copyBuffer(staginBuffer->buffer(), canvasVertexBuffer_->buffer(), size);
    // }

    // {
    //     VkDeviceSize size = sizeof(canvasIndices_[0]) * canvasIndices_.size();

    //     canvasIndexBuffer_ = std::make_unique<Buffer>(physicalDevice_, device_);
    //     canvasIndexBuffer_->size_ = size;
    //     canvasIndexBuffer_->usage_ = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    //     canvasIndexBuffer_->sharingMode_ = queueFamilies_.multiple() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    //     canvasIndexBuffer_->queueFamilyIndexCount_ = static_cast<uint32_t>(queueFamilies_.sets().size());
    //     canvasIndexBuffer_->pQueueFamilyIndices_ = queueFamilies_.sets().data();
    //     canvasIndexBuffer_->memoryProperties_ = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    //     canvasIndexBuffer_->init();

    //     std::unique_ptr<Buffer> staginBuffer = std::make_unique<Buffer>(physicalDevice_, device_);
    //     staginBuffer->size_ = size;
    //     staginBuffer->usage_ = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    //     staginBuffer->sharingMode_ = queueFamilies_.multiple() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    //     staginBuffer->queueFamilyIndexCount_ = static_cast<uint32_t>(queueFamilies_.sets().size());
    //     staginBuffer->pQueueFamilyIndices_ = queueFamilies_.sets().data();
    //     staginBuffer->memoryProperties_ = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    //     staginBuffer->init();

    //     auto data = staginBuffer->map(size);
    //     memcpy(data, canvasIndices_.data(), size);
    //     staginBuffer->unMap();

    //     copyBuffer(staginBuffer->buffer(), canvasIndexBuffer_->buffer(), size);
    // }

    {
        auto data = canvasUniformBuffer_->map(sizeof(UniformBufferObject));
        UniformBufferObject ubo{};
        ubo.proj_ = glm::ortho(-static_cast<float>(swapChain_->width() / 2.0f), static_cast<float>(swapChain_->width() / 2.0f), -static_cast<float>(swapChain_->height() / 2.0), static_cast<float>(swapChain_->height() / 2.0f));
        memcpy(data, &ubo, sizeof(UniformBufferObject));
        canvasUniformBuffer_->unMap();
    }
}

bool Vulkan::checkValidationLayerSupport()  {
    uint32_t count = 0;
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    std::vector<VkLayerProperties> avaliables(count);
    vkEnumerateInstanceLayerProperties(&count, avaliables.data());
    
    std::unordered_set<const char*> requested(validationLayers_.begin(), validationLayers_.end());

    for (const auto& avaliable : avaliables) {
        requested.erase(avaliable.layerName);
    }
    
    return requested.empty();
}

bool Vulkan::checkDeviceExtensionsSupport(VkPhysicalDevice physicalDevice)  {
    uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> avaliables(count);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, avaliables.data());

    std::unordered_set<std::string> requested(deviceExtensions_.begin(), deviceExtensions_.end());
    for (const auto& avaliable : avaliables) {
        requested.erase(avaliable.extensionName);
    }

    return requested.empty();
}

bool Vulkan::deviceSuitable(VkPhysicalDevice physicalDevice) {
    queueFamilies_ = Tools::QueueFamilyIndices::findQueueFamilies(physicalDevice, surface_);
    bool extensionSupport = checkDeviceExtensionsSupport(physicalDevice);
    bool swapChainAdequate = false;
    if (extensionSupport) {
        auto swapChainSupport = Tools::SwapChainSupportDetail::querySwapChainSupport(physicalDevice, surface_);
        swapChainAdequate = !swapChainSupport.formats_.empty() && !swapChainSupport.presentModes_.empty();
    }

    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(physicalDevice, &features);

    return queueFamilies_.compeleted() && extensionSupport && swapChainAdequate && features.samplerAnisotropy;
}

VkSampleCountFlagBits Vulkan::getMaxUsableSampleCount()  {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physicalDevice_, &properties);

    auto count = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;
    if (count & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (count & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (count & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (count & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (count & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (count & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}

VkImageView Vulkan::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)  {
    VkImageView imageView;
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.format = format;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;

    if (vkCreateImageView(device_, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to creat image view!");
    }

    return imageView;
}

VkFormat Vulkan::findDepthFormat() {
    return findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, 
        VK_IMAGE_TILING_OPTIMAL, 
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT  
    );
}

VkFormat Vulkan::findSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags features) {
    VkFormatProperties properties;
    for (const auto& format : formats) {
        vkGetPhysicalDeviceFormatProperties(physicalDevice_, format, &properties);
        if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find support format!");
}

uint32_t Vulkan::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memProperties);
    
    for (size_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & 1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

void Vulkan::copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
    auto commandBuffer = beginSingleTimeCommands();
    
    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer, transferQueue_);
}

VkCommandBuffer Vulkan::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandPool = commandPool_->commanddPool();
    allocateInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device_, &allocateInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void Vulkan::endSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue queue) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device_, commandPool_->commanddPool(), 1, &commandBuffer);
}

void Vulkan::fillColor(uint32_t index) {
    switch (color_) {
    case Color::Write:
        lineVertices_[currentFrame_][index].color_ = glm::vec4(write3_, 0.0f);
        break;
    case Color::Red:
        lineVertices_[currentFrame_][index].color_ = glm::vec4(red3_, 1.0f);
        break;
    case Color::Green:
        lineVertices_[currentFrame_][index].color_ = glm::vec4(green3_, 1.0f);
        break;
    case Color::Blue:
        lineVertices_[currentFrame_][index].color_ = glm::vec4(blue3_, 1.0f);
        break;
    case Color::Black:
        lineVertices_[currentFrame_][index].color_ = glm::vec4(black3_, 1.0f);
        break;
    }
}

void Vulkan::changePoint() {
    auto deltaX = prevCursorRelative_.x - currCursorRelative_.x;
    auto deltaY = prevCursorRelative_.y - currCursorRelative_.y;

    auto a = deltaY / deltaX;
    auto b = prevCursorRelative_.y - a * prevCursorRelative_.x;

    int bx = std::min(prevCursorRelative_.x, currCursorRelative_.x);
    int by = std::min(prevCursorRelative_.y, currCursorRelative_.y);
    int ex = std::max(prevCursorRelative_.x, currCursorRelative_.x);
    int ey = std::max(prevCursorRelative_.y, currCursorRelative_.y);

    bx -= lineWidth_ / 2.0f;
    ex += lineWidth_ / 2.0f;
    by -= lineWidth_ / 2.0f;
    ey += lineWidth_ / 2.0f;

#ifdef DEBUG_CHANGE_POINT
    std::cout << "------change point------" << std::endl;
    std::cout << "delx: " << deltaX << std::endl;
    std::cout << "dely: " << deltaY << std::endl;
    std::cout << "a   : " << a << std::endl;
    std::cout << "b   : " << b << std::endl;
    std::cout << "b   : " << bx << ',' << by << std::endl;
    std::cout << "e   : " << ex << ',' << ey << std::endl;
    std::cout << "prev: " << prevCursorRelative_.x << ',' << prevCursorRelative_.y << std::endl;
    std::cout << "curr: " << currCursorRelative_.x << ',' << currCursorRelative_.y << std::endl;
#endif


    int t = 0;
    int acount = 0;

    if (deltaX == 0) {
        for (int x = bx; x <= ex; x++) {
            for (int y = by; y <= ey; y++) {
                acount++;
                if (std::abs(x - currCursorRelative_.x) <= lineWidth_) {
                    t++;
                    auto index = lineVertexMaps_[currentFrame_][std::make_pair(x, y)];
                    fillColor(index);
                }
            }
        }

#ifdef DEBUG_CHANGE_POINT
        std::cout << "aout: " << acount << std::endl;
        std::cout << "cout: " << t << std::endl;
#endif

        return ;
    }

    for (int x = bx; x <= ex; x++) {
        for (int y = by; y <= ey; y++) {
            acount++;
            if (Tools::pointToLineLength(a, b, x, y) <= lineWidth_) {
                t++;
                auto index = lineVertexMaps_[currentFrame_][std::make_pair(x, y)];
                fillColor(index);
            }
        }
    }
#ifdef DEBUG_CHANGE_POINT
        std::cout << "aout: " << acount << std::endl;
        std::cout << "cout: " << t << std::endl;
#endif
}

VKAPI_ATTR VkBool32 VKAPI_CALL Vulkan::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
        VkDebugUtilsMessageTypeFlagsEXT messageType, 
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
        void* pUserData) {

    std::cerr << "[[Validation Layer]]: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}