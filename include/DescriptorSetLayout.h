#pragma once

#include "vulkan/vulkan_core.h"
#include <vulkan/vulkan.h>

class DescriptorSetLayout {
public:
    DescriptorSetLayout(VkDevice device, VkDescriptorSetLayoutCreateInfo layoutInfo);
    ~DescriptorSetLayout();
    
    VkDescriptorSetLayout get() const { return descriptorSetlayout_; }
private:
    VkDevice device_;
    VkDescriptorSetLayout descriptorSetlayout_;
};