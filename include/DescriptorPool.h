#pragma once

#include "vulkan/vulkan_core.h"
#include <vulkan/vulkan.h>

class DescriptorPool {
public:
    DescriptorPool(VkDevice device, VkDescriptorPoolCreateInfo creatInfo);

    VkDescriptorPool get() const { return descriptorPool_; }
private:
    VkDevice device_;
    VkDescriptorPool descriptorPool_;
};