#include "FrameBuffer.h"
#include "vulkan/vulkan_core.h"
#include <stdexcept>

FrameBuffer::FrameBuffer(VkDevice device, VkFramebufferCreateInfo frameBufferInfo) : device_(device) {
    if (vkCreateFramebuffer(device, &frameBufferInfo, nullptr, &frameBuffer_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create frame buffer!");
    }
}