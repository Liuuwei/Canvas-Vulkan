#include "Vulkan.h"
#include "CommandPool.h"
#include "DescriptorSetLayout.h"
#include "FrameBuffer.h"
#include "GLFW/glfw3.h"
#include "Image.h"
#include "ImageView.h"
#include "Pipeline.h"
#include "PipelineLayout.h"
#include "Swapchain.h"
#include "vulkan/vulkan_core.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <process.h>
#include <setjmp.h>
#include <stdexcept>
#include <iostream>
#include <system_error>
#include <type_traits>
#include <unordered_set>

#include "ShaderModule.h"
#include "Vertex.h"

Vulkan::Vulkan(const std::string& title, uint32_t width, uint32_t height) : width_(width), height_(height), title_(title) {
    initWindow();
    initVulkan();
}

void Vulkan::run() {
    while (!glfwWindowShouldClose(windows_)) {
        glfwPollEvents();
    }
}

void Vulkan::initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

    windows_ = glfwCreateWindow(width_, height_, title_.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(windows_, this);
}

void Vulkan::initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
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

    
    auto extensions = VulkanHelp::getRequiredExtensions();
    VkInstanceCreateInfo creatInfo{};
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    VulkanHelp::populateDebugMessengerCreateInfo(debugCreateInfo, debugCallback);
    creatInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    creatInfo.pApplicationInfo = &appInfo;
    creatInfo.enabledLayerCount = validationLayers_.size();
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
    VulkanHelp::populateDebugMessengerCreateInfo(createInfo, debugCallback);

    if (VulkanHelp::createDebugUtilsMessengerEXT(instance_, &createInfo, nullptr, &debugMessenger_) != VK_SUCCESS) {
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
    deviceInfo.enabledLayerCount = validationLayers_.size();
    deviceInfo.ppEnabledLayerNames = validationLayers_.data();
    deviceInfo.enabledExtensionCount = deviceExtensions_.size();
    deviceInfo.ppEnabledExtensionNames = deviceExtensions_.data();
    deviceInfo.pEnabledFeatures = &features;

    if (vkCreateDevice(physicalDevice_, &deviceInfo, nullptr, &device_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logic device!");
    }
}

void Vulkan::createSwapChain() {
    auto swapChainSupport = VulkanHelp::SwapChainSupportDetail::querySwapChainSupport(physicalDevice_, surface_);
    
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
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;

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

void Vulkan::createDescriptorPool() {
    VkDescriptorPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

    descriptorPool_ = std::make_unique<DescriptorPool>(device_, createInfo);
}

void Vulkan::createDescriptorSetLayout() {
    VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo{};
    descriptorLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

    descriptorSetLayout_ = std::make_unique<DescriptorSetlayout>(device_, descriptorLayoutInfo);
}

void Vulkan::createDescriptorSet() {
    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = descriptorPool_->get();
    allocateInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;

    descriptorSets_.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device_, &allocateInfo, descriptorSets_.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }
}

void Vulkan::createGraphicsPipelines() {
    auto vertexShader = ShaderModule(device_, "../shaders/shader.vert"); 
    auto fragmentShader = ShaderModule(device_, "../shaders/shader.frag");

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
    auto bindingDescription = vertex_->bindingDescription(0);
    auto attributeDescription = vertex_->attributeDescription(0);

    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescription.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology = vertex_->topology();

    VkPipelineViewportStateCreateInfo viewportInfo{};
    viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.viewportCount = 1;
    viewportInfo.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizaInfo{};
    rasterizaInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizaInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizaInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizaInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;

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
    colorBlendAttachmentInfo.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    
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
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = swapChain_->format();
    imageInfo.extent = {swapChain_->extent().width, swapChain_->extent().height, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = msaaSamples_;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    imageInfo.sharingMode = queueFamilies_.multiple() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilies_.sets().size());
    imageInfo.pQueueFamilyIndices = queueFamilies_.sets().data();

    colorImage_ = std::make_unique<Image>(device_, imageInfo);

    VkImageViewCreateInfo imageViewInfo{};
    imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewInfo.image = colorImage_->get();
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewInfo.format = colorImage_->format();
    imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewInfo.subresourceRange.layerCount = 1;
    imageViewInfo.subresourceRange.levelCount = 1;

    colorImageView_ = std::make_unique<ImageView>(device_, imageViewInfo);

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device_, colorImage_->get(), &memRequirements);
    VkMemoryAllocateInfo memoryInfo{};
    memoryInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryInfo.allocationSize = memRequirements.size;
    memoryInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    colorImageMemory_ = std::make_unique<Memory>(device_, memoryInfo);

    vkBindImageMemory(device_, colorImage_->get(), colorImageMemory_->get(), 0);
}

void Vulkan::createDepthResource() {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.format = findDepthFormat();
    imageInfo.extent = {swapChain_->extent().width, swapChain_->extent().height, 1};
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = msaaSamples_;
    imageInfo.sharingMode = queueFamilies_.multiple() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilies_.sets().size());
    imageInfo.pQueueFamilyIndices = queueFamilies_.sets().data();
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;    

    depthImage_ = std::make_unique<Image>(device_, imageInfo);

    VkImageViewCreateInfo imageViewInfo{};
    imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewInfo.image = depthImage_->get();
    imageViewInfo.format = depthImage_->format();
    imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    imageViewInfo.subresourceRange.layerCount = 1;
    imageViewInfo.subresourceRange.levelCount = 1;

    depthImageView_ = std::make_unique<ImageView>(device_, imageViewInfo);    
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
            colorImageView_->get(), 
            swapChain_->imageView(i), 
            depthImageView_->get(),   
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

bool Vulkan::checkValidationLayerSupport() const {
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

bool Vulkan::checkDeviceExtensionsSupport(VkPhysicalDevice physicalDevice) const {
    uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> avaliables(count);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, avaliables.data());

    std::unordered_set<const char*> requested(deviceExtensions_.begin(), deviceExtensions_.end());
    for (const auto& avaliable : avaliables) {
        requested.erase(avaliable.extensionName);
    }

    return requested.empty();
}

bool Vulkan::deviceSuitable(VkPhysicalDevice physicalDevice) {
    queueFamilies_ = VulkanHelp::QueueFamilyIndices::findQueueFamilies(physicalDevice, surface_);
    bool extensionSupport = checkDeviceExtensionsSupport(physicalDevice);
    bool swapChainAdequate = false;
    if (extensionSupport) {
        auto swapChainSupport = VulkanHelp::SwapChainSupportDetail::querySwapChainSupport(physicalDevice, surface_);
        swapChainAdequate = !swapChainSupport.formats_.empty() && !swapChainSupport.presentModes_.empty();
    }

    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(physicalDevice, &features);
    
    return queueFamilies_.compeleted() && extensionSupport && swapChainAdequate && features.samplerAnisotropy;
}

VkSampleCountFlagBits Vulkan::getMaxUsableSampleCount() const {
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

VkImageView Vulkan::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) const {
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

VkFormat Vulkan::findDepthFormat() const {
    return findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, 
        VK_IMAGE_TILING_OPTIMAL, 
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT  
    );
}

VkFormat Vulkan::findSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags features) const {
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

uint32_t Vulkan::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memProperties);
    
    for (size_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & 1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

void Vulkan::copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) const {
    auto commandBuffer = beginSingleTimeCommands();
    
    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer, transferQueue_);
}

VKAPI_ATTR VkBool32 VKAPI_CALL Vulkan::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
        VkDebugUtilsMessageTypeFlagsEXT messageType, 
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
        void* pUserData) {

    std::cerr << "[[Validation Layer]]: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}