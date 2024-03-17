#pragma once

#include "vulkan/vulkan_core.h"
#include <vulkan/vulkan.h>

class FrameBuffer {
public:
    FrameBuffer(VkDevice device, VkFramebufferCreateInfo frameBufferInfo);
    ~FrameBuffer();

    VkFramebuffer get() const { return frameBuffer_; }
private:
    VkDevice device_;
    VkFramebuffer frameBuffer_;
};