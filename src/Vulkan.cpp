#include "Vulkan.h"
#include "Buffer.h"
#include "CommandPool.h"
#include "DescriptorSetLayout.h"
#include "FrameBuffer.h"
#include "GLFW/glfw3.h"
#include "Image.h"
#include "Pipeline.h"
#include "PipelineLayout.h"
#include "Swapchain.h"
#include "Tools.h"
#include "vulkan/vulkan_core.h"
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <memory>
#include <stdexcept>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <unordered_set>
#include <ktx.h>
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
            }
        }

        auto camera = vulkan->camera_;
        camera->onKey(window, key, scancode, action, mods);
    });
    glfwSetCursorPosCallback(windows_, [](GLFWwindow* window, double xpos, double ypos) {
        auto vulkan = reinterpret_cast<Vulkan*>(glfwGetWindowUserPointer(window));

        if (vulkan->LeftButton_) {
            std::string msg = "m" + std::to_string(static_cast<int32_t>(vulkan->swapChain_->width())) + "-"
                                + std::to_string(static_cast<int32_t>(vulkan->swapChain_->height())) + "-"
                                + std::to_string(static_cast<int32_t>(xpos)) + "-"
                                + std::to_string(static_cast<int32_t>(ypos)) + "\r\n\r\n";
            vulkan->tcpConnection_->send(msg);
        }
        
        auto camera = vulkan->camera_;
        camera->onMouseMovement(window, xpos, ypos);
    });
    glfwSetMouseButtonCallback(windows_, [](GLFWwindow* window, int button, int action, int mods) {
        auto vulkan = reinterpret_cast<Vulkan*>(glfwGetWindowUserPointer(window));
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            std::string msg;
            if (action == GLFW_PRESS) {
                vulkan->LeftButton_ = true;
                vulkan->LeftButtonOnce_ = true;
            } else {
                vulkan->LeftButton_ = false;
                vulkan->LeftButtonOnce_ = false;
            }
            vulkan->tcpConnection_->send(msg);
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

    VkSwapchainCreateInfoKHR swapChainInfo{};
    swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainInfo.surface = surface_;
    swapChainInfo.minImageCount = imageCount;
    swapChainInfo.imageFormat = surfaceFormat.format;
    swapChainInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapChainInfo.imageExtent = extent;
    swapChainInfo.imageArrayLayers = 1;
    swapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapChainInfo.imageSharingMode = queueFamilies_.multiple() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    swapChainInfo.queueFamilyIndexCount = queueFamilies_.sets().size();
    swapChainInfo.pQueueFamilyIndices = queueFamilies_.sets().data();
    swapChainInfo.preTransform = swapChainSupport.capabilities_.currentTransform;
    swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainInfo.presentMode = presentMode;
    swapChainInfo.clipped = VK_TRUE;
    swapChainInfo.oldSwapchain = VK_NULL_HANDLE;

    swapChain_ = std::make_unique<SwapChain>(device_, swapChainInfo);
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
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &subpassDependency;
    renderPassInfo.attachmentCount = attachment.size();
    renderPassInfo.pAttachments = attachment.data();

    renderPass_ = std::make_unique<RenderPass>(device_,renderPassInfo);
}

void Vulkan::createUniformBuffers() {
    uniformBuffers_.resize(MAX_FRAMES_IN_FLIGHT);
    auto size = sizeof(UniformBufferObject);

    for (size_t i = 0; i < uniformBuffers_.size(); i++) {
        uniformBuffers_[i] = std::make_unique<myVK::Buffer>(physicalDevice_, device_);
        uniformBuffers_[i]->size_ = size;
        uniformBuffers_[i]->usage_ = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        uniformBuffers_[i]->queueFamilyIndexCount_ = static_cast<uint32_t>(queueFamilies_.sets().size());
        uniformBuffers_[i]->pQueueFamilyIndices_ = queueFamilies_.sets().data();
        uniformBuffers_[i]->memoryProperties_ = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        uniformBuffers_[i]->sharingMode_ = VK_SHARING_MODE_EXCLUSIVE;
        uniformBuffers_[i]->init();

        uniformBuffers_[i]->map(size);
    }
}

void Vulkan::createSamplers() {
    sampler_ = std::make_unique<Sampler>(device_);
    sampler_->addressModeU_ = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_->addressModeV_ = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_->addressModeW_ = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_->minLod_ = 0.0f;
    sampler_->maxLod_ = 1.0f;
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physicalDevice_, &properties);
    sampler_->anisotropyEnable_ = VK_TRUE;
    sampler_->maxAnisotropy_ = properties.limits.maxSamplerAnisotropy;
    sampler_->init();
}

void Vulkan::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 1> poolSizes;
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    createInfo.pPoolSizes = poolSizes.data();
    createInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    descriptorPool_ = std::make_unique<DescriptorPool>(device_, createInfo);
}

void Vulkan::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboBinding{};
    uboBinding.binding = 0;
    uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    std::array<VkDescriptorSetLayoutBinding, 1> bindings = {uboBinding};
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    descriptorSetLayout_ = std::make_unique<DescriptorSetLayout>(device_, layoutInfo);
}

void Vulkan::createDescriptorSet() {
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout_->get());

    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = descriptorPool_->get();
    allocateInfo.descriptorSetCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    allocateInfo.pSetLayouts = descriptorSetLayouts.data();

    descriptorSets_.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device_, &allocateInfo, descriptorSets_.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < descriptorSets_.size(); i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers_[i]->buffer();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets_[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(device_, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

void Vulkan::createVertex() {
    vertex_ = std::make_unique<Plane>();
    // auto t = vertex_->vertices(800.0f, 600.0f);
    // vertices_ = t.first;
    // indices_ = t.second;

    line_ = std::make_unique<Line>();
    lineVertices_.resize(MAX_FRAMES_IN_FLIGHT);
    lineIndices_.resize(MAX_FRAMES_IN_FLIGHT);  
    lineVertexBuffers_.resize(MAX_FRAMES_IN_FLIGHT);
    lineIndexBuffers_.resize(MAX_FRAMES_IN_FLIGHT);  

    lineOffsets_.resize(MAX_FRAMES_IN_FLIGHT);
}

void Vulkan::createGraphicsPipelines() {
    auto vertexShader = ShaderModule(device_, "../shaders/vert.spv"); 
    auto fragmentShader = ShaderModule(device_, "../shaders/frag.spv");

    VkPipelineShaderStageCreateInfo vertexStageInfo{}, fragmentStageInfo{};
    vertexStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexStageInfo.module = vertexShader.get();
    vertexStageInfo.pName = "main";

    fragmentStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentStageInfo.module = fragmentShader.get();
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

    std::array<VkDynamicState, 2> dynamics{
        VK_DYNAMIC_STATE_VIEWPORT, 
        VK_DYNAMIC_STATE_SCISSOR, 
    };
    VkPipelineDynamicStateCreateInfo dynamicInfo{};
    dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicInfo.dynamicStateCount = static_cast<uint32_t>(dynamics.size());
    dynamicInfo.pDynamicStates = dynamics.data();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts = {descriptorSetLayout_->get()};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayout_ = std::make_unique<PipelineLayout>(device_, pipelineLayoutInfo);

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = shaderStages.size();
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineInfo.pViewportState = &viewportInfo;
    pipelineInfo.pRasterizationState = &rasterizaInfo;;
    pipelineInfo.pMultisampleState = &multipleInfo;
    pipelineInfo.pDepthStencilState = &depthStencilInfo;
    pipelineInfo.pColorBlendState = &colorBlendInfo;
    pipelineInfo.pDynamicState = &dynamicInfo;
    pipelineInfo.layout = pipelineLayout_->get();
    pipelineInfo.renderPass = renderPass_->get();

    std::vector<VkGraphicsPipelineCreateInfo> pipelineInfos{pipelineInfo};
    graphicsPipeline_ = std::make_unique<Pipeline>(device_, VK_NULL_HANDLE, pipelineInfos);
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
    
    VkFramebufferCreateInfo frameBufferInfo{};
    frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferInfo.renderPass = renderPass_->get();
    frameBufferInfo.width = swapChain_->extent().width;
    frameBufferInfo.height = swapChain_->extent().height;
    frameBufferInfo.layers = 1;

    for (size_t i = 0; i < frameBuffers_.size(); i++) {
        std::array<VkImageView, 3> attachment = {
            colorImage_->view(), 
            swapChain_->imageView(i), 
            depthImage_->view(),   
        };

        frameBufferInfo.attachmentCount = static_cast<uint32_t>(attachment.size());
        frameBufferInfo.pAttachments = attachment.data();

        frameBuffers_[i] = std::make_unique<FrameBuffer>(device_, frameBufferInfo);
    }
}

void Vulkan::createCommandPool() {
    VkCommandPoolCreateInfo commandPoolInfo{};
    commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolInfo.queueFamilyIndex = queueFamilies_.graphics.value();

    commandPool_ = std::make_unique<CommandPool>(device_, commandPoolInfo);
}

void Vulkan::createCommandBuffers() {
    commandBuffers_.resize(MAX_FRAMES_IN_FLIGHT);
    
    VkCommandBufferAllocateInfo commandBufferInfo{};
    commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferInfo.commandPool = commandPool_->get();
    commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferInfo.commandBufferCount = 1;
    
    for (size_t i = 0; i < commandBuffers_.size(); i++) {
        commandBuffers_[i] = std::make_unique<CommandBuffer>(device_, commandBufferInfo);
    }
}

void Vulkan::createVertexBuffer() {
    // VkDeviceSize size = sizeof(float) * vertices_.size();

    // vertexBuffer_ = std::make_unique<Buffer>(physicalDevice_, device_);
    // vertexBuffer_->size_ = size;
    // vertexBuffer_->usage_ = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    // vertexBuffer_->sharingMode_ = queueFamilies_.multiple() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    // vertexBuffer_->queueFamilyIndexCount_ = static_cast<uint32_t>(queueFamilies_.sets().size());
    // vertexBuffer_->pQueueFamilyIndices_ = queueFamilies_.sets().data();
    // vertexBuffer_->memoryProperties_ = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    // vertexBuffer_->init();

    // std::unique_ptr<Buffer> staginBuffer = std::make_unique<Buffer>(physicalDevice_, device_);
    // staginBuffer->size_ = size;
    // staginBuffer->usage_ = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    // staginBuffer->sharingMode_ = queueFamilies_.multiple() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    // staginBuffer->queueFamilyIndexCount_ = static_cast<uint32_t>(queueFamilies_.sets().size());
    // staginBuffer->pQueueFamilyIndices_ = queueFamilies_.sets().data();
    // staginBuffer->memoryProperties_ = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    // staginBuffer->init();

    // auto data = staginBuffer->map(size);
    // memcpy(data, vertices_.data(), size);
    // staginBuffer->unMap();

    // copyBuffer(staginBuffer->buffer(), vertexBuffer_->buffer(), size);

    for (size_t i = 0; i < lineVertexBuffers_.size(); i++) {
        VkDeviceSize size = sizeof(float) * lineVertices_[i].size();

        lineVertexBuffers_[i] = std::make_unique<myVK::Buffer>(physicalDevice_, device_);
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

        std::unique_ptr<myVK::Buffer> staginBuffer = std::make_unique<myVK::Buffer>(physicalDevice_, device_);
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
    // VkDeviceSize size = sizeof(indices_[0]) * indices_.size();

    // indexBuffer_ = std::make_unique<Buffer>(physicalDevice_, device_);
    // indexBuffer_->size_ = size;
    // indexBuffer_->usage_ = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    // indexBuffer_->sharingMode_ = queueFamilies_.multiple() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    // indexBuffer_->queueFamilyIndexCount_ = static_cast<uint32_t>(queueFamilies_.sets().size());
    // indexBuffer_->pQueueFamilyIndices_ = queueFamilies_.sets().data();
    // indexBuffer_->memoryProperties_ = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    // indexBuffer_->init();

    // std::unique_ptr<Buffer> staginBuffer = std::make_unique<Buffer>(physicalDevice_, device_);
    // staginBuffer->size_ = size;
    // staginBuffer->usage_ = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    // staginBuffer->sharingMode_ = queueFamilies_.multiple() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    // staginBuffer->queueFamilyIndexCount_ = static_cast<uint32_t>(queueFamilies_.sets().size());
    // staginBuffer->pQueueFamilyIndices_ = queueFamilies_.sets().data();
    // staginBuffer->memoryProperties_ = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    // staginBuffer->init();

    // auto data = staginBuffer->map(size);
    // memcpy(data, indices_.data(), size);
    // staginBuffer->unMap();

    // copyBuffer(staginBuffer->buffer(), indexBuffer_->buffer(), size);
    
    for (size_t i = 0; i < lineVertexBuffers_.size(); i++) {
        VkDeviceSize size = sizeof(float) * lineIndices_[i].size();

        lineIndexBuffers_[i] = std::make_unique<myVK::Buffer>(physicalDevice_, device_);
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

        std::unique_ptr<myVK::Buffer> staginBuffer = std::make_unique<myVK::Buffer>(physicalDevice_, device_);
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
        inFlightFences_[i] = std::make_unique<Fence>(device_, fenceInfo);
        imageAvaiableSemaphores_[i] = std::make_unique<Semaphore>(device_, semaphoreInfo);
        renderFinishSemaphores_[i] = std::make_unique<Semaphore>(device_, semaphoreInfo);
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
    renderPassBeginInfo.renderPass = renderPass_->get();
    renderPassBeginInfo.framebuffer = frameBuffers_[imageIndex]->get();
    renderPassBeginInfo.renderArea.extent = swapChain_->extent();
    std::array<VkClearValue, 3> clearValues;
    clearValues[0].color = {0.0f, 0.0f, 0.0f};
    clearValues[1].color = {0.0f, 0.0f, 0.0f};
    clearValues[2].depthStencil = {1.0f, 0};
    renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassBeginInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_->pipeline());

        // std::vector<VkBuffer> vertexBuffer = {vertexBuffer_->buffer()};
        std::vector<VkBuffer> vertexBuffer = {lineVertexBuffers_[currentFrame_]->buffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffer.data(), offsets);

        vkCmdBindIndexBuffer(commandBuffer, lineIndexBuffers_[currentFrame_]->buffer(), 0, VK_INDEX_TYPE_UINT32);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = static_cast<float>(swapChain_->extent().height);
        viewport.width = static_cast<float>(swapChain_->extent().width);
        viewport.height = -static_cast<float>(swapChain_->extent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.extent = swapChain_->extent();
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        std::array<VkDescriptorSet, 1> descriptorSets{descriptorSets_[currentFrame_]};
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_->get(), 0, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);

        // std::cout << "------indices------" << std::endl;
        for (auto p : lineOffsets_[currentFrame_]) {
            // std::cout << p.first << "," << p.second - p.first << "|";
            vkCmdDrawIndexed(commandBuffer, p.second - p.first, 1, p.first, 0, 0);
        }
        // std::cout << std::endl;
        // std::cout << "------vertices------" << std::endl;
        // for (size_t i = 0; i < lineVertices_[currentFrame_].size(); i += 7) {
            // std::cout << lineVertices_[currentFrame_][i] << "," << lineVertices_[currentFrame_][i + 1] << "|";
        // }
        // std::cout << std::endl;
        // std::cout << "------times------" << std::endl << times_ << std::endl;
    
    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to end command buffer!");
    }
}

void Vulkan::draw() {
    timer_.tick();
    camera_->setDeltaTime(timer_.delta());

    vkWaitForFences(device_, 1, inFlightFences_[currentFrame_]->getPtr(), VK_TRUE, UINT64_MAX);

    if (lineVertices_[currentFrame_].size() == 0 && ok_ == false) {
        return ;
    }

    uint32_t imageIndex = 0;
    vkAcquireNextImageKHR(device_, swapChain_->get(), UINT64_MAX, imageAvaiableSemaphores_[currentFrame_]->get(), VK_NULL_HANDLE, &imageIndex);

    vkResetFences(device_, 1, inFlightFences_[currentFrame_]->getPtr());

    vkResetCommandBuffer(commandBuffers_[currentFrame_]->get(), 0);

    updateDrawAssets();

    recordCommadBuffer(commandBuffers_[currentFrame_]->get(), imageIndex);

    std::array<VkSemaphore, 1> waits = {imageAvaiableSemaphores_[currentFrame_]->get()};
    std::array<VkPipelineStageFlags, 1> waitStages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    std::array<VkCommandBuffer, 1> commandBuffers = {commandBuffers_[currentFrame_]->get()};
    std::array<VkSemaphore, 1> signals = {renderFinishSemaphores_[currentFrame_]->get()};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waits.size());
    submitInfo.pWaitSemaphores = waits.data();
    submitInfo.pWaitDstStageMask = waitStages.data();
    submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
    submitInfo.pCommandBuffers = commandBuffers.data();
    submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signals.size());
    submitInfo.pSignalSemaphores = signals.data();

    if (vkQueueSubmit(graphicsQueue_, 1, &submitInfo, inFlightFences_[currentFrame_]->get()) != VK_SUCCESS) {
        throw std::runtime_error("failed to queue submit!");
    }

    std::array<VkSwapchainKHR, 1> swapChains = {swapChain_->get()};

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
    
}

void Vulkan::updateDrawAssets() {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    // ubo.model_ = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -10.0f));
    // ubo.view_ = camera_->getView();
    // ubo.proj_ = camera_->getProj(45.0f, swapChain_->extent().width, swapChain_->extent().height, 0.001f, 1000.0f);
    ubo.proj_ = glm::ortho(-swapChain_->width() / 2.0f, swapChain_->width() / 2.0f, -swapChain_->height() / 2.0f, swapChain_->height() / 2.0f);

    auto data = uniformBuffers_[currentFrame_]->map(sizeof(ubo));    
    memcpy(data, &ubo, sizeof(ubo));
    {   
        if (ok_ == false || LeftButton_ == false) {
            return ;
        }
        std::string msg;
        msg = std::to_string(static_cast<int32_t>(swapChain_->width())) + "-" + std::to_string(static_cast<int32_t>(swapChain_->height())) + "-";
        msg += std::to_string(static_cast<int32_t>(x_)) + "-" + std::to_string(static_cast<int32_t>(y_));
        msg += "\r\n\r\n";
        std::cout << msg << std::endl;
        tcpConnection_->send(msg);
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
        if (lineVertices_[prevFrame].size() > vertices.size()) {
            vertices = lineVertices_[prevFrame];
            indices = lineIndices_[prevFrame];
            indexOffset = lineOffsets_[prevFrame];
        }
        // if (lineIndices_[prevFrame].size() > indices.size()) indices = lineIndices_[prevFrame];
        // if (lineOffsets_[prevFrame].size() > indexOffset.size()) indexOffset = lineOffsets_[prevFrame];
        if (LeftButtonOnce_) {
            times_++;
            LeftButtonOnce_ = false;
            indexOffset.push_back({indices.size(), 0});
        }
        vertices.push_back(static_cast<float>(x_ - swapChain_->width() / 2.0f));
        vertices.push_back(-static_cast<float>(y_ - swapChain_->height() / 2.0f));

        fillColor(vertices);

        vertices.push_back(1.0f);
        vertices.push_back(1.0f);
        indices.push_back(indices.size());    
        if (!indexOffset.empty()) {
            indexOffset.back().second = indices.size();
        }
        
        VkDeviceSize size = sizeof(float) * vertices.size();
        vertexBuffer = std::make_unique<myVK::Buffer>(physicalDevice_, device_);
        vertexBuffer->size_ = size;
        vertexBuffer->usage_ = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        vertexBuffer->sharingMode_ = queueFamilies_.multiple() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
        vertexBuffer->queueFamilyIndexCount_ = static_cast<uint32_t>(queueFamilies_.sets().size());
        vertexBuffer->pQueueFamilyIndices_ = queueFamilies_.sets().data();
        vertexBuffer->memoryProperties_ = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        vertexBuffer->init();

        std::unique_ptr<myVK::Buffer> staginBuffer = std::make_unique<myVK::Buffer>(physicalDevice_, device_);
        staginBuffer->size_ = size;
        staginBuffer->usage_ = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        staginBuffer->sharingMode_ = queueFamilies_.multiple() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
        staginBuffer->queueFamilyIndexCount_ = static_cast<uint32_t>(queueFamilies_.sets().size());
        staginBuffer->pQueueFamilyIndices_ = queueFamilies_.sets().data();
        staginBuffer->memoryProperties_ = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        staginBuffer->init();

        auto data = staginBuffer->map(size);
        memcpy(data, vertices.data(), size);
        staginBuffer->unMap();

        copyBuffer(staginBuffer->buffer(), vertexBuffer->buffer(), size);

        size = sizeof(uint32_t) * indices.size();
        indexBuffer = std::make_unique<myVK::Buffer>(physicalDevice_, device_);
        indexBuffer->size_ = size;
        indexBuffer->usage_ = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        indexBuffer->sharingMode_ = queueFamilies_.multiple() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
        indexBuffer->queueFamilyIndexCount_ = static_cast<uint32_t>(queueFamilies_.sets().size());
        indexBuffer->pQueueFamilyIndices_ = queueFamilies_.sets().data();
        indexBuffer->memoryProperties_ = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        indexBuffer->init();

        staginBuffer = std::make_unique<myVK::Buffer>(physicalDevice_, device_);
        staginBuffer->size_ = size;
        staginBuffer->usage_ = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        staginBuffer->sharingMode_ = queueFamilies_.multiple() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
        staginBuffer->queueFamilyIndexCount_ = static_cast<uint32_t>(queueFamilies_.sets().size());
        staginBuffer->pQueueFamilyIndices_ = queueFamilies_.sets().data();
        staginBuffer->memoryProperties_ = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        staginBuffer->init();

        data = staginBuffer->map(size);
        memcpy(data, indices.data(), size);
        staginBuffer->unMap();

        copyBuffer(staginBuffer->buffer(), indexBuffer->buffer(), size);
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
    allocateInfo.commandPool = commandPool_->get();
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

    vkFreeCommandBuffers(device_, commandPool_->get(), 1, &commandBuffer);
}

void Vulkan::fillColor(std::vector<float>& vertices) {
    switch (color_) {
    case Color::Write:
        vertices.push_back(1.0f);
        vertices.push_back(1.0f);
        vertices.push_back(1.0f);
        break;
    case Color::Red:
        vertices.push_back(1.0f);
        vertices.push_back(0.0f);
        vertices.push_back(0.0f);
        break;
    case Color::Green:
        vertices.push_back(0.0f);
        vertices.push_back(1.0f);
        vertices.push_back(0.0f);
        break;
    case Color::Blue:
        vertices.push_back(0.0f);
        vertices.push_back(0.0f);
        vertices.push_back(1.0f);
        break;
    }
}

void Vulkan::processNetWork(const std::string& msg) {
    if (msg[0] == 'l') {
        if (msg[1] == 'p') {
            ok_ = true;
            LeftButton_ = true;
            LeftButtonOnce_ = true;
        } else {
            LeftButton_ = false;
            LeftButtonOnce_ = false;
        }
        return ;
    }
    if (!LeftButton_) {
        return ;
    }
    
    auto [extent, position] = parseMsg(msg);
    position.first = position.first / extent.first * swapChain_->width();
    position.second = position.second / extent.second * swapChain_->height();

    ok_ = true;
    x_ = position.first;
    y_ = position.second;
}

std::pair<std::pair<float, float>, std::pair<float, float>> Vulkan::parseMsg(const std::string& msg) {
    float width = 0, height = 0, x = 0, y = 0;
    size_t i = 1;
    for (; i < msg.size(); i++) {
        if (msg[i] == '-') {
            break;
        }
        width = width * 10 + msg[i] - '0';
    }
    for (; i < msg.size(); i++) {
        if (msg[i] == '-') {
            break;
        }
        height = height * 10 + msg[i] - '0';
    }
    for (; i < msg.size(); i++) {
        if (msg[i] == '-') {
            break;
        }
        x = x * 10 + msg[i] - '0';
    }
    for (; i < msg.size() && msg[i] != '\r'; i++) {
        if (msg[i] == '-') {
            break;
        }
        y = y * 10 + msg[i] - '0';
    }

    return {{width, height}, {x, y}};
}

VKAPI_ATTR VkBool32 VKAPI_CALL Vulkan::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
        VkDebugUtilsMessageTypeFlagsEXT messageType, 
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
        void* pUserData) {

    std::cerr << "[[Validation Layer]]: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}