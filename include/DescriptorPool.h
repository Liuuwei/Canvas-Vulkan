#pragma once

#include "vulkan/vulkan_core.h"
#include <vulkan/vulkan.h>

class DescriptorPool {
public:
    DescriptorPool(VkDevice device);
    ~DescriptorPool();

    void init();
    VkDescriptorPool descriptorPool() const { return descriptorPool_; }
    void*                    pNext_{};
    VkDescriptorPoolCreateFlags    flags_{};
    uint32_t                       maxSets_{};
    uint32_t                       poolSizeCount_{};
    VkDescriptorPoolSize*    pPoolSizes_{};
private:
    VkDevice device_;
    VkDescriptorPool descriptorPool_;
};