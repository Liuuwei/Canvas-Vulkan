#include "Fence.h"
#include "vulkan/vulkan_core.h"
#include "Tools.h"

Fence::Fence(VkDevice device) : device_(device) {

}

Fence::~Fence() {
    vkDestroyFence(device_, fence_, nullptr);
}

void Fence::init() {
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = flags_;
    fenceInfo.pNext = pNext_;
    VK_CHECK(vkCreateFence(device_, &fenceInfo, nullptr, &fence_));
}