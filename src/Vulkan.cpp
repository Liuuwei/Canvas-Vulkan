#include "Vulkan.h"
#include "Buffer.h"
#include "CommandBuffer.h"
#include "CommandPool.h"
#include "DescriptorPool.h"
#include "DescriptorSetLayout.h"
#include "Editor.h"
#include "FrameBuffer.h"
#include "GLFW/glfw3.h"
#include "Image.h"
#include "Pipeline.h"
#include "PipelineLayout.h"
#include "Swapchain.h"
#include "Tools.h"
#include "vulkan/vulkan_core.h"
#include <cassert>
#include <cmath>
#include <format>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
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
        int inWindow = glfwGetWindowAttrib(window, GLFW_HOVERED);
        if (!inWindow) {
            return ;
        }
        auto vulkan = reinterpret_cast<Vulkan*>(glfwGetWindowUserPointer(window));

        if (action == GLFW_PRESS) {
            vulkan->input(key, scancode, mods);
        }
    });
    glfwSetCursorPosCallback(windows_, [](GLFWwindow* window, double xpos, double ypos) {
        auto vulkan = reinterpret_cast<Vulkan*>(glfwGetWindowUserPointer(window));
    });
    glfwSetMouseButtonCallback(windows_, [](GLFWwindow* window, int button, int action, int mods) {
        auto vulkan = reinterpret_cast<Vulkan*>(glfwGetWindowUserPointer(window));
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
    createEditor();
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
    creatInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
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
    vkGetPhysicalDeviceFeatures(physicalDevice_, &features);
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

    std::vector<VkAttachmentDescription> attachment{colorAttachment, colorAttachmentResolve, depthAttachment};

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
    auto size = sizeof(UniformBufferObject);

    uniformBuffers_ = std::make_unique<Buffer>(physicalDevice_, device_);
    uniformBuffers_->size_ = size;
    uniformBuffers_->usage_ = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    uniformBuffers_->queueFamilyIndexCount_ = static_cast<uint32_t>(queueFamilies_.sets().size());
    uniformBuffers_->pQueueFamilyIndices_ = queueFamilies_.sets().data();
    uniformBuffers_->memoryProperties_ = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    uniformBuffers_->sharingMode_ = VK_SHARING_MODE_EXCLUSIVE;
    uniformBuffers_->init();

    uniformBuffers_->map(size);

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
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physicalDevice_, &properties);
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
    createTextDescriptorPool();
    createCanvasDescriptorPool();
}

void Vulkan::createTextDescriptorPool() {
    std::vector<VkDescriptorPoolSize> poolSizes(2);
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 1;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(dictionary_.size());

    fontDescriptorPool_ = std::make_unique<DescriptorPool>(device_);
    fontDescriptorPool_->poolSizeCount_ = static_cast<uint32_t>(poolSizes.size());
    fontDescriptorPool_->pPoolSizes_ = poolSizes.data();
    fontDescriptorPool_->maxSets_ = 1;
    fontDescriptorPool_->init();
}

void Vulkan::createCanvasDescriptorPool() {
    std::vector<VkDescriptorPoolSize> poolSizes(2);
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 1;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = 1;

    canvasDescriptorPool_ = std::make_unique<DescriptorPool>(device_);
    canvasDescriptorPool_->flags_ = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    canvasDescriptorPool_->poolSizeCount_ = static_cast<uint32_t>(poolSizes.size());
    canvasDescriptorPool_->pPoolSizes_ = poolSizes.data();
    canvasDescriptorPool_->maxSets_ = 1;
    canvasDescriptorPool_->init();
}

void Vulkan::createDescriptorSetLayout() {
    createTextDescriptorSetLayout();
    createCanvasDescriptorSetLayout();
}


void Vulkan::createTextDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboBinding{}, samplerBinding{};
    uboBinding.binding = 0;
    uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    samplerBinding.binding = 1;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.descriptorCount = 1;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::vector<VkDescriptorSetLayoutBinding> bindings = {uboBinding, samplerBinding};

    canvasDescriptorSetLayout_ = std::make_unique<DescriptorSetLayout>(device_);
    canvasDescriptorSetLayout_->bindingCount_ = static_cast<uint32_t>(bindings.size());
    canvasDescriptorSetLayout_->pBindings_ = bindings.data();
    canvasDescriptorSetLayout_->init();
}

void Vulkan::createCanvasDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboBinding{}, samplerBinding{};
    uboBinding.binding = 0;
    uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    samplerBinding.binding = 1;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.descriptorCount = dictionary_.size();
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::vector<VkDescriptorSetLayoutBinding> bindings = {uboBinding, samplerBinding};

    fontDescriptorSetLayout_ = std::make_unique<DescriptorSetLayout>(device_);
    fontDescriptorSetLayout_->bindingCount_ = static_cast<uint32_t>(bindings.size());
    fontDescriptorSetLayout_->pBindings_ = bindings.data();
    fontDescriptorSetLayout_->init();
}

void Vulkan::createDescriptorSet() {
    createTextDescriptorSet();
    createCanvasDescriptorSet();
}

void Vulkan::createTextDescriptorSet() {
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts(1, fontDescriptorSetLayout_->descriptorSetLayout());

    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = fontDescriptorPool_->descriptorPool();
    allocateInfo.descriptorSetCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    allocateInfo.pSetLayouts = descriptorSetLayouts.data();

    if (vkAllocateDescriptorSets(device_, &allocateInfo, &fontDescriptorSet_) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffers_->buffer();
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    
    std::vector<VkDescriptorImageInfo> imageInfos(dictionary_.size());
    for (uint32_t i = 0; i < 128; i++) {
        char c = static_cast<char>(i);
        imageInfos[i].imageView = dictionary_[c].image_->view();
        imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfos[i].sampler = canvasSampler_->sampler();
    }

    std::vector<VkWriteDescriptorSet> descriptorWrites(2);
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = fontDescriptorSet_;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = fontDescriptorSet_;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].descriptorCount = static_cast<uint32_t>(imageInfos.size());
    descriptorWrites[1].pImageInfo = imageInfos.data();
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    vkUpdateDescriptorSets(device_, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Vulkan::createCanvasDescriptorSet() {
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts(1, canvasDescriptorSetLayout_->descriptorSetLayout());

    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = canvasDescriptorPool_->descriptorPool();
    allocateInfo.descriptorSetCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    allocateInfo.pSetLayouts = descriptorSetLayouts.data();

    if (canvasDescriptorSets_ != VK_NULL_HANDLE) {
        VK_CHECK(vkFreeDescriptorSets(device_, canvasDescriptorPool_->descriptorPool(), 1, &canvasDescriptorSets_));
    }
    VK_CHECK(vkAllocateDescriptorSets(device_, &allocateInfo, &canvasDescriptorSets_));

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffers_->buffer();
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkDescriptorImageInfo samplerInfo{};
    samplerInfo.imageView = canvasImage_->view();
    samplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    samplerInfo.sampler = canvasSampler_->sampler();

    std::vector<VkWriteDescriptorSet> descriptorWrites(2);
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = canvasDescriptorSets_;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].pBufferInfo = &bufferInfo;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = canvasDescriptorSets_;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].pImageInfo = &samplerInfo;

    vkUpdateDescriptorSets(device_, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}


void Vulkan::createVertex() {
    canvas_ = std::make_unique<Plane>();
    auto t = canvas_->vertices(0, 0, swapChain_->width(), swapChain_->height());
    canvasVertices_ = t.first;
    canvasIndices_ = t.second;

    font_ = std::make_unique<Font>(fontPath_.c_str(), 48);
}

void Vulkan::createEditor() {
    editor_ = std::make_unique<Editor>(swapChain_->width(), swapChain_->height(), 33);
    
    // for (int i = 0; i < 1000; i ++) {
    //     for (int j = 0; j < 150; j++) {
    //         editor_->insertChar('a');
    //     }
    //     editor_->enter();
    // }
}

void Vulkan::createTextPipeline() {
    auto vertFont = ShaderModule(device_, "../shaders/spv/VertFont.spv"); 
    auto fragFont = ShaderModule(device_, "../shaders/spv/FragFont.spv");

    VkPipelineShaderStageCreateInfo vertexStageInfo{}, fragmentStageInfo{};
    vertexStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexStageInfo.module = vertFont.shader();
    vertexStageInfo.pName = "main";

    fragmentStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentStageInfo.module = fragFont.shader();
    fragmentStageInfo.pName = "main";

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages{vertexStageInfo, fragmentStageInfo};
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    auto bindingDescription = font_->bindingDescription(0);
    auto attributeDescription = font_->attributeDescription(0);

    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescription.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology = font_->topology();

    VkPipelineViewportStateCreateInfo viewportInfo{};
    viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.viewportCount = 1;
    viewportInfo.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizaInfo{};
    rasterizaInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizaInfo.polygonMode = VK_POLYGON_MODE_FILL;
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

    std::vector<VkDynamicState> dynamics{
        VK_DYNAMIC_STATE_VIEWPORT, 
        VK_DYNAMIC_STATE_SCISSOR, 
    };
    VkPipelineDynamicStateCreateInfo dynamicInfo{};
    dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicInfo.dynamicStateCount = static_cast<uint32_t>(dynamics.size());
    dynamicInfo.pDynamicStates = dynamics.data();

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts = {fontDescriptorSetLayout_->descriptorSetLayout()};
    fontPipelineLayout_ = std::make_unique<PipelineLayout>(device_);
    fontPipelineLayout_->setLayoutCount_ = static_cast<uint32_t>(descriptorSetLayouts.size());
    fontPipelineLayout_->pSetLayouts_ = descriptorSetLayouts.data();
    fontPipelineLayout_->init();

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    fontPipeline_ = std::make_unique<Pipeline>(device_);
    fontPipeline_->stageCount_ = shaderStages.size();
    fontPipeline_->pStages_ = shaderStages.data();
    fontPipeline_->pVertexInputState_ = &vertexInputInfo;
    fontPipeline_->pInputAssemblyState_ = &inputAssemblyInfo;
    fontPipeline_->pViewportState_ = &viewportInfo;
    fontPipeline_->pRasterizationState_ = &rasterizaInfo;;
    fontPipeline_->pMultisampleState_ = &multipleInfo;
    fontPipeline_->pDepthStencilState_ = &depthStencilInfo;
    fontPipeline_->pColorBlendState_ = &colorBlendInfo;
    fontPipeline_->pDynamicState_ = &dynamicInfo;
    fontPipeline_->layout_ = fontPipelineLayout_->pipelineLayout();
    fontPipeline_->renderPass_ = renderPass_->renderPass();
    fontPipeline_->init();
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

    std::vector<VkDynamicState> dynamics{
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
    createTextPipeline();
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
        std::vector<VkImageView> attachment = {
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
    commandBuffers_ = std::make_unique<CommandBuffer>(device_);
    commandBuffers_->commandPool_ = commandPool_->commanddPool();
    commandBuffers_->level_ = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBuffers_->commandBufferCount_ = 1;
    commandBuffers_->init();
}

void Vulkan::createVertexBuffer() {
    {
        VkDeviceSize size = sizeof(canvasVertices_[0]) * canvasVertices_.size();

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
    }
}

void Vulkan::createIndexBuffer() {
    {
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
    }
}

void Vulkan::createSyncObjects() {
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    inFlightFences_ = std::make_unique<Fence>(device_);
    inFlightFences_->flags_ = VK_FENCE_CREATE_SIGNALED_BIT;
    inFlightFences_->init();
    imageAvaiableSemaphores_ = std::make_unique<Semaphore>(device_);
    imageAvaiableSemaphores_->init();
    renderFinishSemaphores_ = std::make_unique<Semaphore>(device_);
    renderFinishSemaphores_->init();
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
    std::vector<VkClearValue> clearValues(3);
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

        // chars
        if (editor_->wordCount_ > 0) {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, fontPipeline_->pipeline());

            std::vector<VkDescriptorSet> descriptorSets{fontDescriptorSet_};
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, fontPipelineLayout_->pipelineLayout(), 0, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);

            std::vector<VkBuffer> vertexBuffer = {fontVertexBuffer_->buffer()};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffer.data(), offsets);
            vkCmdBindIndexBuffer(commandBuffer, fontIndexBuffer_->buffer(), 0, VK_INDEX_TYPE_UINT32);
            
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(fontIndices_.size()), 1, 0, 0, 0);
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
    camera_->setDeltaTime(timer_.deltaMilliseconds());


    uint32_t imageIndex = 0;
    vkAcquireNextImageKHR(device_, swapChain_->swapChain(), UINT64_MAX, imageAvaiableSemaphores_->semaphore(), VK_NULL_HANDLE, &imageIndex);

    vkResetFences(device_, 1, inFlightFences_->fencePtr());

    vkResetCommandBuffer(commandBuffers_->commandBuffer(), 0);

    updateDrawAssets();

    recordCommadBuffer(commandBuffers_->commandBuffer(), imageIndex);

    std::vector<VkSemaphore> waits = {imageAvaiableSemaphores_->semaphore()};
    std::vector<VkPipelineStageFlags> waitStages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    std::vector<VkCommandBuffer> commandBuffers = {commandBuffers_->commandBuffer()};
    std::vector<VkSemaphore> signals = {renderFinishSemaphores_->semaphore()};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waits.size());
    submitInfo.pWaitSemaphores = waits.data();
    submitInfo.pWaitDstStageMask = waitStages.data();
    submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
    submitInfo.pCommandBuffers = commandBuffers.data();
    submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signals.size());
    submitInfo.pSignalSemaphores = signals.data();

    if (vkQueueSubmit(graphicsQueue_, 1, &submitInfo, inFlightFences_->fence()) != VK_SUCCESS) {
        throw std::runtime_error("failed to queue submit!");
    }

    std::vector<VkSwapchainKHR> swapChains = {swapChain_->swapChain()};

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

    vkWaitForFences(device_, 1, inFlightFences_->fencePtr(), VK_TRUE, UINT64_MAX);
}

void Vulkan::loadAssets() {
    loadTextures();
    loadChars();
}

void Vulkan::loadTextures() {
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

void Vulkan::loadChars() {
    font_ = std::make_unique<Font>(fontPath_.c_str(), 32);

    for (uint32_t i = 0; i < 128; i++) {
        char c = static_cast<char>(i);
        font_->loadChar(c);
        auto& currentChar = dictionary_[c];

        auto pixel = font_->bitmap().buffer;
        auto texWidth = font_->bitmap().width;
        auto texHeight = font_->bitmap().rows;
        auto offsetX = font_->glyph()->bitmap_left;
        auto offsetY = font_->glyph()->bitmap_top;
        auto advance = font_->glyph()->advance.x / 64;
        VkDeviceSize size = texWidth * texHeight * 1;

        if (pixel == nullptr) {
            texWidth = 15;
            texHeight = 18;
            offsetX = 2;
            offsetY = 18;
            advance = 19;
            size = texWidth * texHeight * 1;
        }

        currentChar.char_ = c;
        currentChar.offsetX_ = offsetX;
        currentChar.offsetY_ = offsetY;
        currentChar.width_ = texWidth;
        currentChar.height_ = texHeight;
        currentChar.advance_ = advance;
        currentChar.color_ = glm::vec3(0.0f, 0.0f, 0.0f);
        currentChar.index_ = i;

        auto& image = currentChar.image_;        
        image = std::make_unique<Image>(physicalDevice_, device_);
        image->imageType_ = VK_IMAGE_TYPE_2D;
        image->arrayLayers_ = 1;
        image->mipLevles_ = 1;
        image->format_ = VK_FORMAT_R8_UNORM;
        image->extent_ = {texWidth, texHeight, 1};
        image->queueFamilyIndexCount_ = static_cast<uint32_t>(queueFamilies_.sets().size());
        image->pQueueFamilyIndices_ = queueFamilies_.sets().data();
        image->sharingMode_ = queueFamilies_.multiple() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
        image->tiling_ = VK_IMAGE_TILING_OPTIMAL;
        image->usage_ = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        image->memoryProperties_ = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        image->viewType_ = VK_IMAGE_VIEW_TYPE_2D;
        image->samples_ = VK_SAMPLE_COUNT_1_BIT;
        image->subresourcesRange_ = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        image->init();

        Buffer staginBuffer(physicalDevice_, device_);
        staginBuffer.size_ = size;
        staginBuffer.queueFamilyIndexCount_ = static_cast<uint32_t>(queueFamilies_.sets().size());
        staginBuffer.pQueueFamilyIndices_ = queueFamilies_.sets().data();
        staginBuffer.sharingMode_ = queueFamilies_.multiple() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
        staginBuffer.usage_ = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        staginBuffer.memoryProperties_ = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        staginBuffer.init();

        auto data = staginBuffer.map(size);
        if (pixel == nullptr) {
            memset(data, 0, size);
        } else {
            memcpy(data, pixel, size);
        }
        staginBuffer.unMap();

        VkImageSubresourceRange range{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        VkBufferImageCopy region{};
        region.imageExtent = {texWidth, texHeight, 1};
        region.imageSubresource = {1, 0, 0, 1};

        auto cmdBuffer = beginSingleTimeCommands();
            Tools::setImageLayout(cmdBuffer, image->image(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, range, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
            vkCmdCopyBufferToImage(cmdBuffer, staginBuffer.buffer(), image->image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
            Tools::setImageLayout(cmdBuffer, image->image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, range, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        endSingleTimeCommands(cmdBuffer, transferQueue_);
    }
}

void Vulkan::updateDrawAssets() {
    UniformBufferObject ubo{};
    ubo.proj_ = glm::ortho(-static_cast<float>(swapChain_->width()) / 2.0f, static_cast<float>(swapChain_->width()) / 2.0f, -static_cast<float>(swapChain_->height()) / 2.0f, static_cast<float>(swapChain_->height()) / 2.0f);
    auto data = uniformBuffers_->map(sizeof(ubo));    
    memcpy(data, &ubo, sizeof(ubo));
    
    {   
        if (editor_->wordCount_ > 0) {
            auto text = std::vector<std::string>(editor_->lines_.begin() + editor_->limit_.up_, editor_->lines_.begin() + editor_->limit_.bottom_);
            auto t = font_->generateTextLines(-static_cast<float>(swapChain_->width()) / 2.0f, static_cast<float>(swapChain_->height()) / 2.0f - editor_->lineHeight_, text, dictionary_, editor_->lineHeight_);
            fontVertices_ = t.first;
            fontIndices_ = t.second;
            
            // font vertices
            VkDeviceSize size = sizeof(fontVertices_[0]) * fontVertices_.size();

            fontVertexBuffer_.reset(nullptr);
            fontVertexBuffer_ = std::make_unique<Buffer>(physicalDevice_, device_);
            fontVertexBuffer_->size_ = size;
            fontVertexBuffer_->usage_ = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            fontVertexBuffer_->queueFamilyIndexCount_ = static_cast<uint32_t>(queueFamilies_.sets().size());
            fontVertexBuffer_->pQueueFamilyIndices_ = queueFamilies_.sets().data();
            fontVertexBuffer_->sharingMode_ = queueFamilies_.sharingMode();
            fontVertexBuffer_->memoryProperties_ = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            fontVertexBuffer_->init();

            auto staginBuffer = std::make_unique<Buffer>(physicalDevice_, device_);
            staginBuffer->size_ = size;
            staginBuffer->usage_ = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            staginBuffer->sharingMode_ = VK_SHARING_MODE_EXCLUSIVE;
            staginBuffer->queueFamilyIndexCount_ = static_cast<uint32_t>(queueFamilies_.sets().size());
            staginBuffer->pQueueFamilyIndices_ = queueFamilies_.sets().data();
            staginBuffer->memoryProperties_ = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
            staginBuffer->init();

            auto data = staginBuffer->map(size);
            memcpy(data, fontVertices_.data(), size);
            staginBuffer->unMap();

            copyBuffer(staginBuffer->buffer(), fontVertexBuffer_->buffer(), size);

            // font index
            size = sizeof(fontIndices_[0]) * fontIndices_.size();

            fontIndexBuffer_.reset(nullptr);
            fontIndexBuffer_ = std::make_unique<Buffer>(physicalDevice_, device_);
            fontIndexBuffer_->size_ = size;
            fontIndexBuffer_->usage_ = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            fontIndexBuffer_->queueFamilyIndexCount_ = static_cast<uint32_t>(queueFamilies_.sets().size());
            fontIndexBuffer_->pQueueFamilyIndices_ = queueFamilies_.sets().data();
            fontIndexBuffer_->sharingMode_ = queueFamilies_.sharingMode();
            fontIndexBuffer_->memoryProperties_ = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            fontIndexBuffer_->init();

            staginBuffer = std::make_unique<Buffer>(physicalDevice_, device_);
            staginBuffer->size_ = size;
            staginBuffer->usage_ = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            staginBuffer->sharingMode_ = VK_SHARING_MODE_EXCLUSIVE;
            staginBuffer->queueFamilyIndexCount_ = static_cast<uint32_t>(queueFamilies_.sets().size());
            staginBuffer->pQueueFamilyIndices_ = queueFamilies_.sets().data();
            staginBuffer->memoryProperties_ = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
            staginBuffer->init();

            data = staginBuffer->map(size);
            memcpy(data, fontIndices_.data(), size);
            staginBuffer->unMap();

            copyBuffer(staginBuffer->buffer(), fontIndexBuffer_->buffer(), size);
        }
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
    editor_->adjust(swapChain_->width(), swapChain_->height());
    createVertexBuffer();
    createIndexBuffer();

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

void Vulkan::processText() {
    if (text_.size() >= 6 && text_.substr(1, 5) == "load:") {
        auto resource = Tools::rmSpace({text_.begin() + 6, text_.end()});
        updateCanvasTexturePath_ = "../textures/" + Tools::rmSpace(resource);
        updateTexture();
    }
}

void Vulkan::updateTexture() {
    int texWidth, texHeight, texChannels;
    auto start = Timer::nowMilliseconds();
    auto pixel = stbi_load(updateCanvasTexturePath_.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    auto end = Timer::nowMilliseconds();
    std::cout << std::format("ms: {}", end - start);
    if (!pixel) {
        return ;
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

    createCanvasDescriptorSet();
}

void Vulkan::input(int key, int scancode, int mods) {
    if (key == 256) {
        editor_->mode_ = Editor::Mode::Command;
        return ;
    }
    if (editor_->mode_ == Editor::Mode::Command && key == 'I') {
        editor_->mode_ = Editor::Mode::Input;
        return ;
    }

    if (editor_->mode_ == Editor::Mode::Command) {
        inputCommand(key, scancode, mods);
    } else {
        inputText(key, scancode, mods);
    }
}

void Vulkan::inputText(int key, int scancode, int mods) {
    char c;
    if (key == GLFW_KEY_BACKSPACE) {
        editor_->backspace();
    } else if (key == GLFW_KEY_CAPS_LOCK) {
        capsLock_ ^= 1;
    } else if (key == GLFW_KEY_ENTER) {
        editor_->enter();
    } else if (key >= 0 && key <= 127) {
        if (mods == GLFW_MOD_SHIFT) {
            c = Tools::charToShiftChar(Tools::keyToChar(key));
        } else {
            if (!capsLock_ && key >= 'A' && key <= 'Z') {
                key += 32; 
            }
            c = Tools::keyToChar(key);
        }
        if (key != 340) {
            editor_->insertChar(c);
        }
    }
}

void Vulkan::inputCommand(int key, int scancode, int mods) {

}

VKAPI_ATTR VkBool32 VKAPI_CALL Vulkan::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
        VkDebugUtilsMessageTypeFlagsEXT messageType, 
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
        void* pUserData) {

    std::cerr << "[[Validation Layer]]: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}