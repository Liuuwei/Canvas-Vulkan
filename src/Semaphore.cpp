#include "Semaphore.h"
#include "vulkan/vulkan_core.h"
#include "Tools.h"

Semaphore::Semaphore(VkDevice device) : device_(device) {

}

Semaphore::~Semaphore() {
    vkDestroySemaphore(device_, semaphore_, nullptr);
}

void Semaphore::init() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreInfo.flags = flags_;
    semaphoreInfo.pNext = pNext_;
    VK_CHECK(vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &semaphore_));
}