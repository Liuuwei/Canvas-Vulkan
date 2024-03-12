#pragma once

#include "vulkan/vulkan_core.h"
#include <vulkan/vulkan.h>

class RenderPass {
public:
    RenderPass(VkDevice device, VkRenderPassCreateInfo renderPassInfo);
    VkRenderPass get() const { return renderPass_; }
private:
    VkDevice device_;
    VkRenderPass renderPass_;
};