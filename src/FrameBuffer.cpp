#include "FrameBuffer.h"
#include "vulkan/vulkan_core.h"
#include "Tools.h"

FrameBuffer::FrameBuffer(VkDevice device) : device_(device) {

}

FrameBuffer::~FrameBuffer() {
    vkDestroyFramebuffer(device_, frameBuffer_, nullptr);
}

void FrameBuffer::init() {
    VkFramebufferCreateInfo frameBufferInfo{};
    frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferInfo.flags = flags_;
    frameBufferInfo.pNext = pNext_;
    frameBufferInfo.renderPass = renderPass_;
    frameBufferInfo.width = width_;
    frameBufferInfo.height = height_;
    frameBufferInfo.layers = layers_;
    frameBufferInfo.attachmentCount = attachmentCount_;
    frameBufferInfo.pAttachments = pAttachments_;
    VK_CHECK(vkCreateFramebuffer(device_, &frameBufferInfo, nullptr, &frameBuffer_));
}