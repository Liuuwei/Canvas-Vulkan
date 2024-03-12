#pragma once

#include "vulkan/vulkan_core.h"
#include <vulkan/vulkan.h>

#include <vector>

class DescriptorSetlayout {
public:
    DescriptorSetlayout(VkDevice device, VkDescriptorSetLayoutCreateInfo layoutInfo);
    VkDescriptorSetLayout get() const { return descriptorSetlayout_; }
private:
    VkDevice device_;
    VkDescriptorSetLayout descriptorSetlayout_;
};