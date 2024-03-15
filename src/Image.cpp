#include "Image.h"
#include "vulkan/vulkan_core.h"
#include "Tools.h"

Image::Image(VkPhysicalDevice physicalDevice, VkDevice device) : physicalDevice_(physicalDevice), device_(device) {
    
}

void Image::init() {
    VkImageCreateInfo imageInfo{};
    VkMemoryAllocateInfo memoryInfo{};
    VkMemoryRequirements memRequirement;
    VkImageViewCreateInfo viewInfo{};

    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.flags = flags_;
    imageInfo.imageType = imageType_;
    imageInfo.format = format_;
    imageInfo.extent = extent_;
    imageInfo.mipLevels = mipLevles_;
    imageInfo.arrayLayers = arrayLayers_;
    imageInfo.samples = samples_;
    imageInfo.tiling = tiling_;
    imageInfo.usage = usage_;
    imageInfo.sharingMode = sharingMode_;
    imageInfo.queueFamilyIndexCount = queueFamilyIndexCount_;
    imageInfo.pQueueFamilyIndices = pQueueFamilyIndices_;
    
    VK_CHECK(vkCreateImage(device_, &imageInfo, nullptr, &image_));

    vkGetImageMemoryRequirements(device_, image_, &memRequirement);

    memoryInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryInfo.allocationSize = memRequirement.size;
    memoryInfo.memoryTypeIndex = Tools::findMemoryType(physicalDevice_, memRequirement.memoryTypeBits, memoryProperties_);

    VK_CHECK(vkAllocateMemory(device_, &memoryInfo, nullptr, &memory_));

    vkBindImageMemory(device_, image_, memory_, 0);

    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image_;
    viewInfo.viewType = viewType_;
    viewInfo.format = format_;
    viewInfo.subresourceRange = subresourcesRange_;

    VK_CHECK(vkCreateImageView(device_, &viewInfo, nullptr, &view_));
}