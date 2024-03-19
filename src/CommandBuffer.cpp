#include "CommandBuffer.h"
#include "vulkan/vulkan_core.h"
#include "Tools.h"

CommandBuffer::CommandBuffer(VkDevice device) : device_(device) {

}

CommandBuffer::~CommandBuffer() {
    vkFreeCommandBuffers(device_, commandPool_, 1, &commandBuffer_);
}

void CommandBuffer::init() {
    VkCommandBufferAllocateInfo cmdBufferAllocateInfo{};
    cmdBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufferAllocateInfo.pNext = pNext_;
    cmdBufferAllocateInfo.level = level_;
    cmdBufferAllocateInfo.commandBufferCount = commandBufferCount_;
    cmdBufferAllocateInfo.commandPool = commandPool_;
    VK_CHECK(vkAllocateCommandBuffers(device_, &cmdBufferAllocateInfo, &commandBuffer_));
}