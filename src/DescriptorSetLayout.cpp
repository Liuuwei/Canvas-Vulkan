#include "DescriptorSetLayout.h"
#include "vulkan/vulkan_core.h"
#include "Tools.h"

DescriptorSetLayout::DescriptorSetLayout(VkDevice device) : device_(device) {

}

DescriptorSetLayout::~DescriptorSetLayout() {
    vkDestroyDescriptorSetLayout(device_, descriptorSetlayout_, nullptr);
}

void DescriptorSetLayout::init() {
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
    descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutInfo.flags = flags_;
    descriptorSetLayoutInfo.pNext = pNext_;
    descriptorSetLayoutInfo.bindingCount = bindingCount_;
    descriptorSetLayoutInfo.pBindings = pBindings_;
    VK_CHECK(vkCreateDescriptorSetLayout(device_, &descriptorSetLayoutInfo, nullptr, &descriptorSetlayout_));
}