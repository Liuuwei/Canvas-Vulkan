#include "Buffer.h"
#include "vulkan/vulkan_core.h"
#include "Tools.h"

Buffer::Buffer(VkPhysicalDevice physicalDevice, VkDevice device) : physicalDevice_(physicalDevice), device_(device) {
}

Buffer::~Buffer() {
    vkDestroyBuffer(device_, buffer_, nullptr);
    vkFreeMemory(device_, memory_, nullptr);
}

void Buffer::init() {
    VkBufferCreateInfo bufferInfo{};
    VkMemoryAllocateInfo memoryInfo{};
    VkMemoryRequirements memRequirements;

    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size_;
    bufferInfo.usage = usage_;
    bufferInfo.queueFamilyIndexCount = queueFamilyIndexCount_;
    bufferInfo.pQueueFamilyIndices = pQueueFamilyIndices_;
    bufferInfo.sharingMode = sharingMode_;

    VK_CHECK(vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer_));

    vkGetBufferMemoryRequirements(device_, buffer_, &memRequirements);

    memoryInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryInfo.allocationSize = memRequirements.size;
    memoryInfo.memoryTypeIndex = Tools::findMemoryType(physicalDevice_, memRequirements.memoryTypeBits, memoryProperties_);

    VK_CHECK(vkAllocateMemory(device_, &memoryInfo, nullptr, &memory_));

    VK_CHECK(vkBindBufferMemory(device_, buffer_, memory_, 0));
}