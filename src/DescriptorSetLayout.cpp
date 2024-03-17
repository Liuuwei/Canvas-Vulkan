#include "DescriptorSetLayout.h"
#include "vulkan/vulkan_core.h"
#include <stdexcept>

DescriptorSetLayout::DescriptorSetLayout(VkDevice device, VkDescriptorSetLayoutCreateInfo layoutInfo) : device_(device) {
    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetlayout_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

DescriptorSetLayout::~DescriptorSetLayout() {
    vkDestroyDescriptorSetLayout(device_, descriptorSetlayout_, nullptr);
}