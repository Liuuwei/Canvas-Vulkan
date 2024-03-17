#include "Semaphore.h"
#include "vulkan/vulkan_core.h"
#include <stdexcept>

Semaphore::Semaphore(VkDevice device, VkSemaphoreCreateInfo createInfo) : device_(device) {
    if (vkCreateSemaphore(device, &createInfo, nullptr, &semaphore_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create semaphore!");
    }
}

Semaphore::~Semaphore() {
    vkDestroySemaphore(device_, semaphore_, nullptr);
}