#pragma once

#include "vulkan/vulkan_core.h"
#include <vulkan/vulkan.h>

class DescriptorSetLayout {
public:
    DescriptorSetLayout(VkDevice device);
    ~DescriptorSetLayout();
    
    void init();
    VkDescriptorSetLayout descriptorSetLayout() const { return descriptorSetlayout_; }
    void*                            pNext_{};
    VkDescriptorSetLayoutCreateFlags       flags_{};
    uint32_t                               bindingCount_{};
    VkDescriptorSetLayoutBinding*    pBindings_{};
private:
    VkDevice device_;
    VkDescriptorSetLayout descriptorSetlayout_;
};