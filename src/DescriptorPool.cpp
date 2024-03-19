#include "DescriptorPool.h"
#include "vulkan/vulkan_core.h"
#include "Tools.h"

DescriptorPool::DescriptorPool(VkDevice device) : device_(device) {

}

DescriptorPool::~DescriptorPool() {
    vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
}

void DescriptorPool::init() {
    VkDescriptorPoolCreateInfo descriptorPoolInfo{};
    descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.flags = flags_;
    descriptorPoolInfo.pNext = pNext_;
    descriptorPoolInfo.maxSets = maxSets_;
    descriptorPoolInfo.poolSizeCount = poolSizeCount_;
    descriptorPoolInfo.pPoolSizes = pPoolSizes_;
    VK_CHECK(vkCreateDescriptorPool(device_, &descriptorPoolInfo, nullptr, &descriptorPool_));
}