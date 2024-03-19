#pragma once

#include "vulkan/vulkan_core.h"
#include <vulkan/vulkan.h>

class FrameBuffer {
public:
    FrameBuffer(VkDevice device);
    ~FrameBuffer();

    void init();
    VkFramebuffer frameBuffer() const { return frameBuffer_; }
    void*                 pNext_{};
    VkFramebufferCreateFlags    flags_{};
    VkRenderPass                renderPass_{};
    uint32_t                    attachmentCount_{};
    VkImageView*          pAttachments_{};
    uint32_t                    width_{};
    uint32_t                    height_{};
    uint32_t                    layers_{};
private:
    VkDevice device_;
    VkFramebuffer frameBuffer_;
};